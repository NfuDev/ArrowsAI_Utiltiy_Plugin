// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowsMissionComponent.h"


// Sets default values for this component's properties
UArrowsMissionComponent::UArrowsMissionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UArrowsMissionComponent::BeginPlay()
{
	Super::BeginPlay();

	StartNewMission(StartupMission);
	// ...

}


// Called every frame
void UArrowsMissionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (CurrentMission)
	{
		CurrentMission->MissionTick_Implementation(DeltaTime);
	   	CurrentMission->MissionTick(DeltaTime);
	}
	// ...
}

void UArrowsMissionComponent::StartNewMission(TSubclassOf<UArrowsMissionObject> NewMission)
{
	if (IsValid(NewMission))
	{
		CurrentMission = NewObject<UArrowsMissionObject>(this, NewMission);

		CurrentMission->MissionComponent = this;
		CurrentMission->MissionBegin_Implementation(false);
		CurrentMission->MissionBegin(false);
	}
}

void UArrowsMissionComponent::RestartMission()
{
	if (CurrentMission)
	{
		if (CurrentMission->CurrentMissionState == EMissionState::InProgress)
		{
			/*forcing the mission to end and fail*/
			CurrentMission->CurrentMissionState = EMissionState::Failed;
			CurrentMission->MissionFaluireType = EMissionFaluireType::ForcedFail;
			CurrentMission->MissionEnd(false);
			

			FTimerHandle RestartHandle;
			GetWorld()->GetTimerManager().SetTimer(RestartHandle, CurrentMission, &UArrowsMissionObject::DelayedRestartFade, CurrentMission->AutoCallsDelay, false);
			return;
		}
		UArrowsMissionObject* PreviousMesssion = CurrentMission;

		CurrentMission = NewObject<UArrowsMissionObject>(this, CurrentMission->GetClass());

		CurrentMission->MissionComponent = this;
		CurrentMission->AssossiatedActors = PreviousMesssion->AssossiatedActors;
		CurrentMission->MissionBegin_Implementation(true);
		CurrentMission->MissionBegin(true);
		MissionRestarted.Broadcast();
	}
}

void UArrowsMissionComponent::ForceMissionFade()
{
	if (CurrentMission)
	{
		CurrentMission->ForceFadeAnimation();
	}
}

void UArrowsMissionComponent::GoToNextMission()
{
	if (CurrentMission)
	{
		if (CurrentMission->CurrentMissionState != EMissionState::InProgress)
		{
			StartNewMission(CurrentMission->NextMission);
		}
	}
	
}