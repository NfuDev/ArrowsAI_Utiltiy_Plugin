// Fill out your copyright notice in the Description page of Project Settings.


#include "MissionAction.h"
#include "Kismet/KismetMathLibrary.h"
#include "TimerManager.h"

UMissionAction::UMissionAction()
{
	ActionText = FText::FromString("Put Your Mission Action Text Here");
	ActionCount = 1;
}

void UMissionAction::OnReciveActivation_Implementation(UArrowsMissionObject* SourceMission, int32 Count)
{
	//..
}

void UMissionAction::OnReciveFinish_Implementation(UArrowsMissionObject* SourceMission, int32 Count)
{
	//..
}

UWorld* UMissionAction::GetWorld() const
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