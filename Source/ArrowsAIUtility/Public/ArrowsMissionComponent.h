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
	UFUNCTION(BlueprintCallable)
	void StartNewMission(TSubclassOf<UArrowsMissionObject> NewMission);

	

		
};
