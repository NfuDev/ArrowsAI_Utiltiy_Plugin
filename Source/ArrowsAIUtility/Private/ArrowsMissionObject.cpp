// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowsMissionObject.h"
#include "MissionAction.h"

void UArrowsMissionObject::MissionBegin_Implementation()
{
	InitActionStates();
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
		 
		if (CheckStatesForSucess())//we only care here for true since false just means the player did not finish all tasks yet, the timer will take care of the failed mission case
		{
			MissionEnd(true);
		}
		
	}
}


// using the post init to simulate begin play which it should be called when game starts and in this case when this object is constructed Put Your Mission Start Logics Here
void UArrowsMissionObject::PostInitProperties()
{
	Super::PostInitProperties();
	if (GetWorld())
	{

		MissionBegin_Implementation();
		MissionBegin();
		
	}
}

// Tick Event used from engine tickable interface
void UArrowsMissionObject::Tick(float DeltaTime)
{

	if (LastFrameNumberWeTicked == GFrameCounter)
	{
			return;
	}

	MissionTick(DeltaTime);

	LastFrameNumberWeTicked = GFrameCounter;
}

/*Making Sure That The UObject Got A World Reference First Cuz UObjects Are Basic Unreal Classes They Dont Know About The World By Them Self , And They Get The Outter Class When They Are Constructed 
And The Outter Class Tells it About The World*/
UWorld* UArrowsMissionObject::GetWorld() const
{
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
		UMissionAction* ItiratorObject = State.MissionAction.GetDefaultObject();

	   if (ItiratorObject->Countable && !State.Done)
	   {
		   for (auto& preformedAction : TempDoneActions)
		   {
			   UMissionAction* PreformedActionObject = preformedAction.GetDefaultObject();

			   if (PreformedActionObject == ItiratorObject)
			   {
				   State.Count--;

				   if (State.Count <= 0)
				   {
					   State.Done = true;
					   State.Count = 0;
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