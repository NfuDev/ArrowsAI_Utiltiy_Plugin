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

		UMissionAction();
	
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText ActionText;
		
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool Countable;

	UPROPERTY(EditAnywhere , BlueprintReadOnly,  meta = (EditCondition = "Countable == true", EditConditionHides))
	int32 ActionCount;

	/*if this action is used as a black listed action , should it instantly cause the mission to fail or it sould wait for other black listed actions to happen*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool InstantFail;

};
