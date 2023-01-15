// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowsPathAI.generated.h"

class UTextRenderComponent;

UCLASS()
class ARROWSAIUTILITY_API AArrowsPathAI : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AArrowsPathAI();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category = "Arrows AI Utilities")
	class USplineComponent* PatrolingPath;

	UPROPERTY()
	TArray<UTextRenderComponent*> AllPoints;

	UPROPERTY()
    UTextRenderComponent* PointTXT;

	UFUNCTION(CallInEditor)
	void MarkPoint();
};
