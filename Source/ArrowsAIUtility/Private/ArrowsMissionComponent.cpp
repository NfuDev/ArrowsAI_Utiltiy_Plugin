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

	CurrentMission =  NewObject<UArrowsMissionObject>(this, NewMission);

	CurrentMission->MissionComponent = this;
	CurrentMission->MissionBegin_Implementation(false);
	CurrentMission->MissionBegin(false);
	if (CurrentMission)
	{
	/*	if (GEngine)
		{
			FString mes = "Start New Mission Is Called, new mission successfully started";
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, mes);
		}*/
	}

}

void UArrowsMissionComponent::RestartMission()
{
	CurrentMission = NewObject<UArrowsMissionObject>(this, CurrentMission->GetClass());

	CurrentMission->MissionComponent = this;
	CurrentMission->MissionBegin_Implementation(true);
	CurrentMission->MissionBegin(true);
}

void UArrowsMissionComponent::ForceMissionFade()
{
	if (CurrentMission)
	{
		CurrentMission->ForceFadeAnimation();
	}
}