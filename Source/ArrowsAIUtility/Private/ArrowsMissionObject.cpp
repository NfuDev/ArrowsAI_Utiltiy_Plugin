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
}

void UArrowsMissionObject::MissionBegin_Implementation(bool WasRestarted)
{
	bWasRestarted = WasRestarted;
	InitActionStates();
	UArrowsMissionObject* ClassDefaultObject = Cast<UArrowsMissionObject>(this->GetClass()->GetDefaultObject());
	

	if (MissionType == EMissionType::Timed)
	{
		
		GetWorld()->GetTimerManager().SetTimer(MissionTimer, this, &UArrowsMissionObject::MissionTimeOver, MissionTime, false);
		CurrentMissionState = EMissionState::InProgress;
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
			MissionScreenFade(1.0f);//FadeWidget->PlayFadeAnimatoin(EUMGSequencePlayMode::Reverse);
			OnMissionRestart_Implementation();
			OnMissionRestart();//so the user can clean all things related to the mission and start over // moved here so the blueprint class can handle it's logics during the fade since after the fade ends we lose reference
			GetWorld()->GetTimerManager().SetTimer(RestartCounter, this, &UArrowsMissionObject::TriggerRestartCounter, AutoCallsDelay, false);
			return;
		}
		else
		{
			OnMissionRestart_Implementation();
			OnMissionRestart();
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
	if (AssossiatedActors.Contains(Source) && CurrentMissionState == EMissionState::InProgress && !MissionBlackListedActions.Contains(PreformedAction))
	{
		UpdateMissionStates(PreformedAction);

		if (CheckStatesForSucess())//we only care here for true since false just means the player did not finish all tasks yet, the timer will take care of the failed mission case
		{
			MissionEnd_Implementation(true);
			MissionEnd(true);
			CurrentMissionState = EMissionState::Succeeded;

				
			GetWorld()->GetTimerManager().ClearTimer(MissionTimer);
			MissionTimer.Invalidate();
		}
	}

	else if (MissionBlackListedActions.Contains(PreformedAction))
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

void UArrowsMissionObject::MissionTimeOver()
{
	MissionFaluireType = EMissionFaluireType::Timer;

	MissionEnd_Implementation(false);
	MissionEnd(false);
	CurrentMissionState = EMissionState::Failed;
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
		

		if (GetWorld()->GetTimerManager().IsTimerActive(MissionTimer))
		{
			 totalRemaining = CountDown ? FMath::RoundToInt(GetWorld()->GetTimerManager().GetTimerRemaining(MissionTimer)) : FMath::RoundToInt(GetWorld()->GetTimerManager().GetTimerElapsed(MissionTimer));
		}
		    
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

	TArray<TSubclassOf<UMissionAction>> TempStates;
	
	TArray<int32> Indexes;


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

	OnTaskActivated(MissionActionsState[0].MissionAction, MissionActionsState[0].GetMissionDefaults()->Countable, MissionActionsState[0].TotalCount);
}

bool UArrowsMissionObject::CheckStatesForSucess()
{
	bool bAllDone;
	//check for all done here
	for (auto& State : MissionActionsState)
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

	if (MissionRequiredActions.Contains(PreformedAction))
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
						OnActionDone(PreformedAction, State.GetMyIndex(MissionActionsState));
					}
					
				}

				else
				{
					if (!State.Done)
					{
						State.Done = true;
						OnActionDone(PreformedAction, State.GetMyIndex(MissionActionsState));
					}
				}
			}
			
		}

		OnMissionUpdates();
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

TArray<FMissionActionStates> UArrowsMissionObject::GetMissionStatues(EMissionStatusType StatusType)
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

void UArrowsMissionObject::OnActionDone(TSubclassOf<UMissionAction> DoneAction, int32 Index)//the done action is the previous action, i actually dont need it here now but i have some ideas for it later
{
	if (MissionActionsState.IsValidIndex(Index + 1))
	{
		FMissionActionStates State = MissionActionsState[Index + 1];
		UMissionAction* ActionClassDefaults = State.MissionAction.GetDefaultObject();

		OnTaskActivated(State.MissionAction, ActionClassDefaults->Countable, State.TotalCount);
	}
}

void UArrowsMissionObject::OnTaskActivated_Implementation(TSubclassOf<UMissionAction> ActivatedAction, bool bIsCountable, int32 Count)
{
	//..
}