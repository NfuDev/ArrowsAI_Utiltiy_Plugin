// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowsTransformHelperPawn.h"
#include "ArrowsMissionObject.h"
#include "Engine/Classes/Kismet/GameplayStatics.h"

// Sets default values
AArrowsTransformHelperPawn::AArrowsTransformHelperPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CapsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule Component"));
	Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow Component"));
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard Component"));

	SetRootComponent(CapsuleComponent);
	Arrow->SetupAttachment(GetRootComponent());
	BillboardComponent->SetupAttachment(GetRootComponent());

	VariableName = FName("StartLocation");//setting the default value to be the player start
}

// Called when the game starts or when spawned
void AArrowsTransformHelperPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AArrowsTransformHelperPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AArrowsTransformHelperPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AArrowsTransformHelperPawn::SetMissionSpawnTransform()
{
	FStructProperty* TransformProperty = FindFProperty<FStructProperty>(MissionClass, VariableName);//cuz in UnrealType.h they said FindField is deprecated so use FindFProperty

	if (TransformProperty)
	{
		UArrowsMissionObject* Src = MissionClass.GetDefaultObject();
		FTransform* ValuePtr = TransformProperty->ContainerPtrToValuePtr<FTransform>(Src);
		if (ValuePtr)
		{
			*ValuePtr = GetActorTransform();
		}
	}

	else
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, "variable Was Not Found, Please Create One First");
		}
	}

}

void AArrowsTransformHelperPawn::FillSpawnTransformArray()
{
	UArrowsMissionObject* Src = MissionClass.GetDefaultObject();
	TArray<AActor*> helpersList;
	UGameplayStatics::GetAllActorsOfClass(this, this->GetClass(), helpersList);
	

	FArrayProperty* TransformArrayProperty = FindFProperty<FArrayProperty>(MissionClass, ArrayVariableName);//directly find array properties better than trying to find if the property was an array or single
	if(TransformArrayProperty)
	{
		TArray<FTransform>* FoundArrayRef = TransformArrayProperty->ContainerPtrToValuePtr<TArray<FTransform>>(Src);
		FoundArrayRef->Empty();//remove old data

		for (auto& Itr : helpersList)
		{
			FoundArrayRef->Add(Itr->GetActorTransform());

		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, FString::Printf(TEXT("Property was Found, [ %d ] where Added"), FoundArrayRef->Num()));
		}
	}

  else
  {
     if (GEngine)
      {
	  GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, "Property Was Not found");
      }
  }

}