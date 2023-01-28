// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowsMissionObject.h"
#include "Kismet/KismetMathLibrary.h"

void UArrowsMissionObject::MissionBegin_Implementation()
{
	InitActionStates();

	if (MissionType == EMissionType::Timed)
	{
		
		GetWorld()->GetTimerManager().SetTimer(MissionTimer, this, &UArrowsMissionObject::MissionTimeOver, MissionTime, false);

		if (GEngine)
		{
				GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, "Parent BeginPlay");
		}
		CurrentMissionState = EMissionState::InProgress;
	}

}

void UArrowsMissionObject::MissionTick_Implementation(float DeltaTime)
{

   if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Green, "Parent tick");
	}


}

void  UArrowsMissionObject::MissionEnd_Implementation(bool Success)
{
	//..
}

void  UArrowsMissionObject::OnMissionUpdates_Implementation()
{
	//..
}


void UArrowsMissionObject::MissionActionPreformed(AActor* Source, TSubclassOf<UMissionAction> PreformedAction)
{
	if (AssossiatedActors.Contains(Source) && CurrentMissionState == EMissionState::InProgress && !MissionBlackListedActions.Contains(PreformedAction))
	{
		UpdateMissionStates(PreformedAction);

		if (CheckStatesForSucess())//we only care here for true since false just means the player did not finish all tasks yet, the timer will take care of the failed mission case
		{
			MissionEnd(true);
			CurrentMissionState = EMissionState::Succeeded;

				
			GetWorld()->GetTimerManager().ClearTimer(MissionTimer);
			MissionTimer.Invalidate();
		}
	}

	else if (MissionBlackListedActions.Contains(PreformedAction))
	{
		MissionEnd(false);
		CurrentMissionState = EMissionState::Failed;


		GetWorld()->GetTimerManager().ClearTimer(MissionTimer);
		MissionTimer.Invalidate();
	}
}

void UArrowsMissionObject::MissionTimeOver()
{
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
		float totalRemaining;

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


void UArrowsMissionObject::GetActionInfo(FMissionActionStates ActionState, EActionInfoGetType GetterType, FText& ActionText, int32& _Count, bool& _Done)
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
			ActionText = FText::FromString(FString::Printf(TEXT("%s [ %d ]"), *TempText, TempCount));
		}

		else
		{
			ActionText = FText::FromString(FString::Printf(TEXT("%s [ %d / %d ]"), *TempText, ActionState.TotalCount, TempCount));
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

					State.Done = State.Count <= 0;
				}

				else
				{
					State.Done = true;
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

//void UArrowsMissionObject::Tick(float DeltaTime)
//{
//	// it is not used , we get our tick from the component now, this interface causes to many issues , i put it down here just to inheret the blueprint context for gameplay statics functions 
//}