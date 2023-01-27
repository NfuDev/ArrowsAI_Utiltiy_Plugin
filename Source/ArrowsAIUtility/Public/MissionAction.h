// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MissionAction.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class ARROWSAIUTILITY_API UMissionAction : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere)
	FText ActionText;
		
	UPROPERTY(EditAnywhere)
	bool Countable;

};
