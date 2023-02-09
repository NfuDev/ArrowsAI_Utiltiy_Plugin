// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MissionAction.generated.h"

/**
 * 
 */
class UArrowsMissionObject;

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

	/*Called From The Active mission , when this action is activated
	* @param SourceMission the currently active mission that has this action recently activated , you can use the mission version of this event or use this to isolate actions logics from missions
	* @param Count the total count of this action , it may not be the same as the Action count in this class , if you have multiple instances of this action in any array in the main misson it gives thier total
	*/
	UFUNCTION(BlueprintNativeEvent, meta = (AllowPrivateAcess = true))
	void OnReciveActivation(UArrowsMissionObject* SourceMission,  int32 Count);

	/*Called When this action is done , keep in mind those actions calls wont remain for long time since the reference is temperary, once it is lost the object will be garbage collected*/
	UFUNCTION(BlueprintNativeEvent, meta = (AllowPrivateAcess = true))
	void OnReciveFinish(UArrowsMissionObject* SourceMission, int32 Count);

	virtual void OnReciveActivation_Implementation(UArrowsMissionObject* SourceMission, int32 Count);
	virtual void OnReciveFinish_Implementation(UArrowsMissionObject* SourceMission, int32 Count);


	UWorld* GetWorld() const;
};
