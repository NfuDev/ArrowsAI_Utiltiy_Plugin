// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowsMissionObject.h"
#include "Kismet/KismetMathLibrary.h"
#include "ArrowsMissionComponent.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

UArrowsMissionObject::UArrowsMissionObject()
{
	auto FoundFadeWidgetClass = ConstructorHelpers::FClassFinder<UMissionFadeWidget>(TEXT("/ArrowsAIUtility/Fade.Fade_C"));

	if (FoundFadeWidgetClass.Class)
	{
		FadeWidgetClass = FoundFadeWidgetClass.Class;
	}

	MissionInPlace = true;
	bWasRestarted = false;
	AutoRestart = true;
	AutoGoNextMission = true;
	AutoCallsDelay = 3.0f;
	DisableMissionFade = false;
}

void UArrowsMissionObject::MissionBegin_Implementation(bool WasRestarted)
{
	CurrentMissionState = EMissionState::InProgress;
	bWasRestarted = WasRestarted;
	InitActionStates();
	UArrowsMissionObject* ClassDefaultObject = Cast<UArrowsMissionObject>(this->GetClass()->GetDefaultObject());
	

	if (MissionType == EMissionType::Timed)
	{
		TimeCounter = CountDown ? MissionTime : 0.0f;
		GetWorld()->GetTimerManager().SetTimer(MissionTimer, this, &UArrowsMissionObject::MissionTimeOver, 1.0f, true);
	}
	
	//logics to dynamiclly get the place where the mission started so when we restart we set to this location , but if the misson was not in place then this variable should be set by the user in the editor
	if (MissionInPlace && !bWasRestarted)
	{
	  ClassDefaultObject->StartLocation = MissionComponent->GetOwner()->GetActorTransform();// setting the value on the default class instead of instanced pointer so next time when restarting it will be right value
	}
	
	//setting fade widget reference and starup fade
	if (IsValid(FadeWidgetClass))
	{
		FadeWidget = Cast<UMissionFadeWidget>(CreateWidget(GetWorld(), FadeWidgetClass, FName("FadeWidget")));

		if (FadeWidget)
		{
			FadeWidget->AddToViewport(0);

			if (!MissionInPlace || bWasRestarted)
			{
				//set the player to start transform here, and use maybe a timer for when the set is done call the fade below there, 
				MissionComponent->GetOwner()->SetActorTransform(StartLocation);
				FadeWidget->PlayFadeAnimatoin(EUMGSequencePlayMode::Forward);
			}

			else if (MissionInPlace && !bWasRestarted)
			{
				FadeWidget->PlayFadeAnimatoin(EUMGSequencePlayMode::Forward, 100.0f);//so we dont see the fade we play it so fast , if we did not play it when the mission start this will make in place missions get a black screen
			}

		}
	}
	
	//ClassDefaultObject->bWasRestarted = false;
}

void UArrowsMissionObject::MissionTick_Implementation(float DeltaTime)
{

  
}

void  UArrowsMissionObject::MissionEnd_Implementation(bool Success)
{
	//..
	if (!Success)
	{
		if (AutoRestart)
		{
			FTimerHandle RestartHandle;
			GetWorld()->GetTimerManager().SetTimer(RestartHandle, this, &UArrowsMissionObject::DelayedRestartFade, AutoCallsDelay, false);
			return;
		}
		else
		{
			//OnMissionRestart_Implementation(); i dont want to call the restart event here, first i was thinking to give the user  the ability to have some logics when mission is 
			// faild so i call the event and dont call the restart, but he can handle this on mission end event no need for this one
			//OnMissionRestart();
			return;
		}
    }

	else if (AutoGoNextMission)
	{
		UArrowsMissionObject* NextMissionClassDefaults = NextMission.GetDefaultObject();

		if (NextMissionClassDefaults->MissionInPlace)
		{
			MissionComponent->StartNewMission(NextMission);
		}
		
		else
		{
			FTimerHandle FadeTimerHandle;
			GetWorld()->GetTimerManager().SetTimer(FadeTimerHandle, this, &UArrowsMissionObject::DelayedMissionFade, 2.0f, false);
		}

	}

}

void  UArrowsMissionObject::DelayedStartNewMission()
{
	MissionComponent->StartNewMission(NextMission);
}

void  UArrowsMissionObject::DelayedMissionFade()
{
	MissionScreenFade(1.0f);
	FTimerHandle NextMissionTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(NextMissionTimerHandle, this, &UArrowsMissionObject::DelayedStartNewMission, AutoCallsDelay, false);
}

void  UArrowsMissionObject::DelayedRestartFade()
{
	MissionScreenFade(1.0f);
	OnMissionRestart_Implementation();
	OnMissionRestart();//so the user can clean all things related to the mission and start over // moved here so the blueprint class can handle it's logics during the fade since after the fade ends we lose reference
	GetWorld()->GetTimerManager().SetTimer(RestartCounter, this, &UArrowsMissionObject::TriggerRestartCounter, AutoCallsDelay, false);
}

void  UArrowsMissionObject::OnMissionUpdates_Implementation()
{
	//..
}

void  UArrowsMissionObject::OnMissionRestart_Implementation()
{
	//..

}

void  UArrowsMissionObject::ForceFadeAnimation()
{
	if (FadeWidget)
	{
		FadeWidget->PlayFadeAnimatoin(EUMGSequencePlayMode::Forward);
	}

}

void  UArrowsMissionObject::MissionScreenFade(float Rate)
{
	FadeWidget->PlayFadeAnimatoin(EUMGSequencePlayMode::Reverse, Rate);
}

//Delay The restart
void  UArrowsMissionObject::TriggerRestartCounter()//this is happening after the screen goes dark after fade animation
{
	UArrowsMissionObject* ClassDefaultObject = Cast<UArrowsMissionObject>(this->GetClass()->GetDefaultObject());
	
	MissionComponent->GetOwner()->SetActorTransform(StartLocation);
	MissionComponent->RestartMission();
}

void UArrowsMissionObject::MissionActionPreformed(AActor* Source, TSubclassOf<UMissionAction> PreformedAction)
{
	//those to prevent game crash if the user forgot to feed any actions to the lists 
	bool UpdateStateCondition = MissionActionsState.Num() > 0 && MissionRequiredActions.Contains(PreformedAction);
	bool UpdateBlackListCondtion = BlackListedActionsStates.Num() > 0 && MissionBlackListedActions.Contains(PreformedAction);
	bool CallBounceCondition = MissionBounce.Num() > 0 && BounceActionsStates.Num() > 0 && MissionBounce.Contains(PreformedAction);

	if (UpdateStateCondition && AssossiatedActors.Contains(Source) && CurrentMissionState == EMissionState::InProgress && !MissionBlackListedActions.Contains(PreformedAction))
	{
		UpdateMissionStates(PreformedAction);

		if (CheckIfAllDone(MissionActionsState))//we only care here for true since false just means the player did not finish all tasks yet, the timer will take care of the failed mission case
		{
			MissionEnd_Implementation(true);
			MissionEnd(true);
			CurrentMissionState = EMissionState::Succeeded;

				
			GetWorld()->GetTimerManager().ClearTimer(MissionTimer);
			MissionTimer.Invalidate();
		}

	}

	else if (UpdateBlackListCondtion && AssossiatedActors.Contains(Source) && CurrentMissionState == EMissionState::InProgress)
	{
		UpdateBlackListedStatues(PreformedAction);
		UMissionAction* ActionDefaults = PreformedAction.GetDefaultObject();

		if (ActionDefaults->InstantFail || CheckIfAllDone(BlackListedActionsStates))
		{
			FailureCauseAction = PreformedAction;
			MissionFaluireType = EMissionFaluireType::DoneAction;

			MissionEnd_Implementation(false);
			MissionEnd(false);
			CurrentMissionState = EMissionState::Failed;


			GetWorld()->GetTimerManager().ClearTimer(MissionTimer);
			MissionTimer.Invalidate();
		}

	}

	else if (CallBounceCondition && AssossiatedActors.Contains(Source) && CurrentMissionState == EMissionState::InProgress)
	{
		int32 Index = GetActionIndex(BounceActionsStates, PreformedAction);
		if (!BounceActionsStates[Index].Done)//to make sure we get the bounce only once
		{
			OnActionDone(PreformedAction, Index, false);
			BounceActionsStates[Index].Done = true;
		}
	}

}

void UArrowsMissionObject::MissionTimeOver()
{
	TimeCounter = CountDown? --TimeCounter : ++TimeCounter;
	bool timeOverCondition = CountDown ? TimeCounter <= 0 : TimeCounter >= MissionTime;
	if (timeOverCondition)
	{
		GetWorld()->GetTimerManager().ClearTimer(MissionTimer);
		MissionTimer.Invalidate();
		TimeCounter = CountDown ? 0.0f :MissionTime;

		MissionFaluireType = EMissionFaluireType::Timer;

		MissionEnd_Implementation(false);
		MissionEnd(false);
		CurrentMissionState = EMissionState::Failed;
	}
}

void UArrowsMissionObject::PauseMission(bool Pause)
{
		if (Pause)
		{
			if (MissionType == EMissionType::Timed)
			{
				GetWorld()->GetTimerManager().PauseTimer(MissionTimer);
			}
				
			CurrentMissionState = EMissionState::Paused;
		}
		else
		{
			if (MissionType == EMissionType::Timed)
			{
				GetWorld()->GetTimerManager().UnPauseTimer(MissionTimer);
			}
			
			CurrentMissionState = EMissionState::InProgress;
		}
		
}


void UArrowsMissionObject::GetMissionTime(EMissionTimerType TimerType, FText& Time, float& Counter)
{
	if (MissionType == EMissionType::Regulared)
	{
		Time = FText::FromString("Mission Is Not Timed!");
		Counter = 0.0f;
	}

    else if(MissionType == EMissionType::Timed) 
	{
		

		//if (GetWorld()->GetTimerManager().IsTimerActive(MissionTimer))
		//{
		//	// totalRemaining = CountDown ? FMath::RoundToInt(GetWorld()->GetTimerManager().GetTimerRemaining(MissionTimer)) : FMath::RoundToInt(GetWorld()->GetTimerManager().GetTimerElapsed(MissionTimer));
		//	totalRemaining = TimeCounter;
		//}

		    totalRemaining = TimeCounter;
			//totalRemaining = GetWorld()->GetTimerManager().IsTimerActive(MissionTimer) ? totalRemaining : TimeWhenFinished;//so we dont get -1 after the missin is failed

			float seconds;
			int32 Minutes = UKismetMathLibrary::FMod((totalRemaining), 60.0f, seconds);

			FText TempText = FText::FromString(FString::Printf(TEXT("[ %d : %d ]"), Minutes, FMath::RoundToInt(seconds)));
			
			Time = TimerType == EMissionTimerType::TotalSeconds? FText::FromString(FString::FromInt(totalRemaining)) : TempText;
			Counter = totalRemaining;
	}
}


void UArrowsMissionObject::GetActionInfo(FMissionActionStates ActionState, EActionInfoGetType GetterType, bool FixTextFormat, FText& ActionText, int32& _Count, bool& _Done)
{
	UMissionAction* BaseClass = ActionState.MissionAction.GetDefaultObject();

	bool bDecrement = (GetterType == EActionInfoGetType::Done_Decrementally || GetterType == EActionInfoGetType::Total_Done_Decrementally);
	
	bool bIsCountable = BaseClass->Countable;
	FString TempText = BaseClass->ActionText.ToString();
	int32 TempCount = bDecrement? ActionState.Count : (ActionState.TotalCount - ActionState.Count);
	
	_Count = TempCount;
	_Done = ActionState.Done;

	if (bIsCountable)
	{
		if (GetterType == EActionInfoGetType::Done_Decrementally || GetterType == EActionInfoGetType::Done_Increamentally)
		{
			ActionText = FixTextFormat? FText::FromString(FString::Printf(TEXT("%s ] %d ["), *TempText, TempCount)) : FText::FromString(FString::Printf(TEXT("%s [ %d ]"), *TempText, TempCount));
		}

		else
		{
			ActionText = FixTextFormat? FText::FromString(FString::Printf(TEXT("%s ] %d / %d ["), *TempText, ActionState.TotalCount, TempCount)) : FText::FromString(FString::Printf(TEXT("%s [ %d / %d ]"), *TempText, ActionState.TotalCount, TempCount));
		}
	}
	else
	{
		ActionText = FText::FromString(TempText);
	}

}

void UArrowsMissionObject::InitActionStates()
{
	MissionActionsState.Empty();
	BlackListedActionsStates.Empty();
	BounceActionsStates.Empty();

	TArray<TSubclassOf<UMissionAction>> TempStates;
	
	TArray<int32> Indexes;

	if (MissionRequiredActions.Num() > 0)
	{

		for (auto& itr : MissionRequiredActions)
		{
			UMissionAction* ActionObject = itr.GetDefaultObject();

			if (TempStates.Contains(itr))// if it is found in the temp array meaning it was added so we increament the count
			{
				int32 _Index = TempStates.Find(itr);

				int32 ElementToEditIndex = Indexes.Find(_Index);


				MissionActionsState[ElementToEditIndex].Count += ActionObject->ActionCount;
				MissionActionsState[ElementToEditIndex].TotalCount = MissionActionsState[ElementToEditIndex].Count;
			}

			else // if it is not found we create a new element and add to the states for UI display
			{
				int32 _Index = TempStates.Add(itr);

				FMissionActionStates TempActionState;
				TempActionState.MissionAction = itr;
				TempActionState.Count = ActionObject->ActionCount;
				TempActionState.TotalCount = ActionObject->ActionCount;
				TempActionState.Done = false;

				MissionActionsState.Add(TempActionState);

				Indexes.Add(_Index);

			}
		}

		if (StatusType == EMissionStatusType::ShowAll)
		{
			for (FMissionActionStates State : MissionActionsState)
			{
				OnTaskActivated(State.MissionAction, State.GetMissionDefaults()->Countable, State.TotalCount);//activate all actions if the getter type was set to (show all) so the activation will work for all of them
			}
		}

		if (StatusType == EMissionStatusType::OneByOne)
		{
			OnTaskActivated(MissionActionsState[0].MissionAction, MissionActionsState[0].GetMissionDefaults()->Countable, MissionActionsState[0].TotalCount);
			ActivatedActions.Add(MissionActionsState[0].MissionAction);
		}

	}
	//create black list state for each black list action in the list , not taking in mind any doublicates
	TArray<TSubclassOf<UMissionAction>> TempBlackListedActions;
	if (MissionBlackListedActions.Num() > 0)
	{

		for (auto& itr : MissionBlackListedActions)
		{
			if (!TempBlackListedActions.Contains(itr))
			{
				FMissionActionStates _NewState;
				_NewState.MissionAction = itr;
				_NewState.Done = false;
				BlackListedActionsStates.Add(_NewState);
				TempBlackListedActions.Add(itr);//removing all instances of this item so we get rid of all doubles 
			}
		}
	}

	// create bounce states for the bounce actions , the states are needed just to calculate the total count if the user used multiple instance of same action so we combine the count, here i dont think it is 
	// nessessary to do i guess 
	TempStates.Empty();//reusing the past temp array that is used for main actions 
	Indexes.Empty();
	if (MissionBounce.Num() > 0)
	{

		for (auto& itr : MissionBounce)
		{
			if (TempStates.Contains(itr))
			{
				//the reason for this logic and the one used with the main actoins is we have two arrays that are not growing in the same pasing , maybe 3 actions are added to the same state , 
				//so (0 ,1 ,2) are all found in the index (0) in the states so we find where the state using saved index when we created the state for the action, planning to refactor the logic into one function
				int32 _Index = TempStates.Find(itr);
				int32 ElementToEdit = Indexes.Find(_Index);
				BounceActionsStates[ElementToEdit].Count += itr.GetDefaultObject()->ActionCount;
				BounceActionsStates[ElementToEdit].TotalCount = BounceActionsStates[ElementToEdit].Count;
			}
			else
			{
				int32 _Index = TempStates.Add(itr);

				FMissionActionStates TempActionState;
				TempActionState.MissionAction = itr;
				TempActionState.Count = itr.GetDefaultObject()->ActionCount;
				TempActionState.TotalCount = itr.GetDefaultObject()->ActionCount;
				TempActionState.Done = false;

				BounceActionsStates.Add(TempActionState);

				Indexes.Add(_Index);

			}
		}
	}
}


bool UArrowsMissionObject::CheckIfAllDone(TArray<FMissionActionStates> _ActionsArray)
{
	bool bAllDone;
	//check for all done here
	for (auto& State : _ActionsArray)
	{
		bAllDone = State.Done;

		if (!bAllDone)
		{
			return bAllDone;
		}
	}

	return true;
}

void UArrowsMissionObject::UpdateMissionStates(TSubclassOf<UMissionAction> PreformedAction)
{
	UMissionAction* PreformedActionClassDefaults = PreformedAction.GetDefaultObject();

	bool UpdateCondition = StatusType == EMissionStatusType::OneByOne ? ActivatedActions.Contains(PreformedAction) : true;

	if (MissionRequiredActions.Contains(PreformedAction) && UpdateCondition)
	{
		for (auto& State : MissionActionsState)
		{
			if (State.MissionAction == PreformedAction)
			{

				if (PreformedActionClassDefaults->Countable)
				{
					State.Count = State.Count <= 0 ? 0 : --State.Count;

				    if (State.Count <= 0 && !State.Done)
					{
						State.Done = true;

						 OnActionDone(PreformedAction, State.GetMyIndex(MissionActionsState), true);
						
					}
					
				}

				else
				{
					if (!State.Done)
					{
						State.Done = true;
					    OnActionDone(PreformedAction, State.GetMyIndex(MissionActionsState), true);
					}
				}
			}
			
		}

		OnMissionUpdates();
	}
}


void UArrowsMissionObject::UpdateBlackListedStatues(TSubclassOf<UMissionAction> PreformedAction)
{
	if (MissionBlackListedActions.Contains(PreformedAction))
	{
		for (auto& blackListedState : BlackListedActionsStates)
		{
			if (blackListedState.MissionAction == PreformedAction)
			{
				if(!blackListedState.Done) blackListedState.Done = true;
			}
		}
	}

}


void UArrowsMissionObject::AddAssossiatedActor(AActor* Source)
{
	AssossiatedActors.AddUnique(Source);
}

UWorld* UArrowsMissionObject::GetWorld() const
{
	// Return pointer to World from object owner, if we don’t work in editor
	if (GIsEditor && !GIsPlayInEditorWorld)
	{
		return nullptr;
	}
	else if (GetOuter())
	{
		return GetOuter()->GetWorld();
	}
	return nullptr;
}

TArray<FMissionActionStates> UArrowsMissionObject::GetMissionStatues()
{
	if (StatusType == EMissionStatusType::ShowAll)
	{
		return MissionActionsState;
	}
	else
	{
		TArray<FMissionActionStates> TempStatus;
		TArray<FMissionActionStates> TempNotDoneStatus;

		if (MissionActionsState.Num() != 0)
		{
			//spletting the actions to done and not done , since we need only one action that is not done and all the done , to show all done first and then show one not done , to get
			//the behaviour of showing first not done and when it is done we show only one other not done so we get one by one 
			for (auto& itr : MissionActionsState)
			{
				if (itr.Done)
				{
					TempStatus.Add(itr);
				}
				else
				{
					TempNotDoneStatus.Add(itr);
				}
			}

			if (TempNotDoneStatus.IsValidIndex(0))
			{
				TempStatus.Add(TempNotDoneStatus[0]);
				return TempStatus;
			}

		}

		return TempStatus; // here if the rest of the actions were all done so they will be in the temp and the not done array will be empty so we return here
	}


}

void UArrowsMissionObject::OnActionDone(TSubclassOf<UMissionAction> DoneAction, int32 Index, bool bIsMainAction)//the done action is the previous action, i actually dont need it here now but i have some ideas for it later
{
	//check for main action activation , cuz this function is used to handle the optional actions too so we check if the done action is main one so we activate the next
	if (bIsMainAction)
	{
		if (StatusType == EMissionStatusType::OneByOne)
		{
			if (MissionActionsState.IsValidIndex(Index + 1))
			{
				FMissionActionStates State = MissionActionsState[Index + 1];
				UMissionAction* ActionClassDefaults = State.MissionAction.GetDefaultObject();

				OnTaskActivated(State.MissionAction, ActionClassDefaults->Countable, State.TotalCount);
				ActivatedActions.Add(State.MissionAction);
			}
		}

		OnTaskDone(DoneAction, DoneAction.GetDefaultObject()->Countable, MissionActionsState[Index].TotalCount);
	}

	else
	{
		OnTaskDone(DoneAction, DoneAction.GetDefaultObject()->Countable, BounceActionsStates[Index].TotalCount);
	}
	
}

void UArrowsMissionObject::OnTaskActivated_Implementation(TSubclassOf<UMissionAction> DoneAction, bool bIsCountable, int32 Count)
{
	//..
}

void UArrowsMissionObject::OnTaskDone_Implementation(TSubclassOf<UMissionAction> DoneAction, bool bIsCountable, int32 Count)
{
	//..
}
