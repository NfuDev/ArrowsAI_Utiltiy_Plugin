// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArrowsMissionObject.h"
#include "Delegates/Delegate.h"
#include "ArrowsMissionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUpdateAssociation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMissionPassed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMissionFailed);

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ARROWSAIUTILITY_API UArrowsMissionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UArrowsMissionComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission Settings")
	TSubclassOf<UArrowsMissionObject> StartupMission;

	UPROPERTY(BlueprintReadWrite, Category = "Mission Settings")
	UArrowsMissionObject* CurrentMission;

	/*called when a certain mission is restarted, so you can have a global logic when mission restarts, implemented in any other class by binding events to this delegate*/
	UPROPERTY(BlueprintAssignable, Category = "Component Core")
	FUpdateAssociation MissionRestarted;

	/*called when mission is passed , used to have global logics for mission pass , like showing general "You Win" if the mission has it own ui to show when winning and all mission have 
	some in common ui they show when win , then use this delegate to call the in common ui creation logics , or any other logics needed when mission passed*/
	UPROPERTY(BlueprintAssignable, Category = "Component Core")
	FOnMissionPassed OnMissionPassed;

	/*same as for mission passed but for mission failed*/
	UPROPERTY(BlueprintAssignable, Category = "Component Core")
	FOnMissionFailed OnMissionFailed;

	/*Start New Mission*/
	UFUNCTION(BlueprintCallable, Category = "Component Core")
	void StartNewMission(TSubclassOf<UArrowsMissionObject> NewMission);

	/*call this if you want a mission to start after a fade , since the start fade only occurs on missions that changes player location so if the first mission does not change the location and
	you need a fade , use this function to force it*/
	UFUNCTION(BlueprintCallable, Category = "Component Core")
	void ForceMissionFade();
		
	/*Called To Restart Any Currently Running Mission*/
	UFUNCTION(BlueprintCallable, Category = "Component Core")
    void RestartMission();

	/*Called To Manually Go To Next Mission if The [Go To Next Mission] boolean in the mission object is false, wont work if mission is in progress , this 
	function is meant to be used when you do not want to use [Auto Go To Next Mission]*/
	UFUNCTION(BlueprintCallable, Category = "Component Core")
	void GoToNextMission();
};
