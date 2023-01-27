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
   //..
}

void  UArrowsMissionObject::MissionEnd_Implementation(bool Success)
{
	//..
}


void UArrowsMissionObject::MissionActionPreformed(AActor* Source, TSubclassOf<UMissionAction> PreformedAction)
{
	if (AssossiatedActors.Contains(Source))
	{
		DoneActions.Add(PreformedAction);

		if (CurrentMissionState == EMissionState::InProgress)
		{
			if (CheckStatesForSucess())//we only care here for true since false just means the player did not finish all tasks yet, the timer will take care of the failed mission case
			{
				MissionEnd(true);
				CurrentMissionState = EMissionState::Succeeded;

				
				GetWorld()->GetTimerManager().ClearTimer(MissionTimer);
				MissionTimer.Invalidate();

			}
		}
		
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
		float totalRemaining = CountDown ? FMath::RoundToInt(GetWorld()->GetTimerManager().GetTimerRemaining(MissionTimer)) : FMath::RoundToInt(GetWorld()->GetTimerManager().GetTimerElapsed(MissionTimer));
			
			float seconds;
			int32 Minutes = UKismetMathLibrary::FMod((totalRemaining), 60.0f, seconds);

			FText TempText = FText::FromString(FString::Printf(TEXT("[ %d : %d ]"), Minutes, FMath::RoundToInt(seconds)));
			
			Time = TimerType == EMissionTimerType::TotalSeconds? FText::FromString(FString::FromInt(totalRemaining)) : TempText;
			Counter = totalRemaining;
	}
}


void UArrowsMissionObject::GetActionInfo(FMissionActionStates ActionState, FText& ActionText, int32& _Count, bool& _Done)
{
	UMissionAction* BaseClass = ActionState.MissionAction.GetDefaultObject();

	bool bIsCountable = BaseClass->Countable;
	FString TempText = BaseClass->ActionText.ToString();
	int32 TempCount = ActionState.Count;
	
	_Count = TempCount;
	_Done = ActionState.Done;

	if (bIsCountable)
	{
		ActionText = FText::FromString(FString::Printf(TEXT("%s [ %d ]"), *TempText, TempCount));
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
		if (TempStates.Contains(itr))// if it is found in the temp array meaning it was added so we increament the count
		{
			int32 _Index = TempStates.Find(itr);

			int32 ElementToEditIndex = Indexes.Find(_Index);

			MissionActionsState[ElementToEditIndex].Count++;
		}

		else // if it is not found we create a new element and add to the states for UI display
		{
			int32 _Index = TempStates.Add(itr);

			FMissionActionStates TempActionState;
			TempActionState.MissionAction = itr;
			TempActionState.Count = 1;
			TempActionState.Done = false;

			MissionActionsState.Add(TempActionState);

			Indexes.Add(_Index);
		}
	}

}

bool UArrowsMissionObject::CheckStatesForSucess()
{
	TArray<TSubclassOf<UMissionAction>> TempDoneActions;
	TempDoneActions = DoneActions;//making a copy of the done actions cuz the calculation for success will ruin the source array so we do this 

	bool bAllDone = false;
	//editing the progress from the done actions and reflect them on the actions state
	for (auto& State : MissionActionsState)
	{

		if (!State.Done)
		{
			UMissionAction* ItiratorObject = State.GetMissionDefaults();

			if (ItiratorObject->Countable)
			{
				for (auto& preformedAction : TempDoneActions)
				{
					UMissionAction* PreformedActionObject = preformedAction.GetDefaultObject();

					if (preformedAction == State.MissionAction)
					{
						State.Count--;

						if (State.Count <= 0)
						{
							State.Done = true;
							State.Count = 0;
							TempDoneActions.Remove(State.MissionAction);//so we dont get dublicate results and once action counted as two and everything goes out of sync
						}

					}
				}
			}

			else
			{
				if (TempDoneActions.Contains(State.MissionAction))
				{
					State.Done = true;
				}
			}
		}
	  
	}

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

void UArrowsMissionObject::Tick(float DeltaTime)
{
	// it is not used , we get our tick from the component now, this interface causes to many issues , i put it down here just to inheret the blueprint context for gameplay statics functions 
}