// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "ArrowsTransformHelperPawn.generated.h"

class UArrowsMissionObject;

UCLASS()
class ARROWSAIUTILITY_API AArrowsTransformHelperPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AArrowsTransformHelperPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UCapsuleComponent* CapsuleComponent;
	UArrowComponent* Arrow;
	UBillboardComponent* BillboardComponent;

	/*the mission class in the content browser you want to edit the variable on*/
	UPROPERTY(EditAnywhere, Category = "A_Transform Helper")
	TSubclassOf<UArrowsMissionObject> MissionClass;

	/*the name of the variable to edit*/
	UPROPERTY(EditAnywhere, Category = "Single Transform Helper")
	FName VariableName;

	/*Use This to set the transform in any mission you want to set player start transform or even a transform variable to use for spawning enemies , this pawn automaically 
	set the variable in the default class, by taking it's world transform and push it to the selected mission class, default value is player start for mission that are not marked [in place], but you can change it 
	to add different location variable value for other logics , maybe COIN spawn location*/
	UFUNCTION(CallInEditor, Category = "Single Transform Helper")
	void SetMissionSpawnTransform();

	/*the name of the Array variable to edit*/
	UPROPERTY(EditAnywhere, Category = "Array Transform Helper")
    FName ArrayVariableName;

	/*Fill Transform Array for you, provide array name , make sure to not have any other [TransformHelper pawn] in the world other than the 
	ones you want to add their transform to the location for your personal logic*/
	UFUNCTION(CallInEditor, Category = "Array Transform Helper")
	void FillSpawnTransformArray();
};
