// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArrowsMissionObject.h"
#include "ArrowsMissionComponent.generated.h"


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
