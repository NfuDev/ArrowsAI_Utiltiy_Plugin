// Fill out your copyright notice in the Description page of Project Settings.


#include "ArrowsPathAI.h"
#include "Components/SplineComponent.h"
#include "Components/TextRenderComponent.h"

// Sets default values
AArrowsPathAI::AArrowsPathAI()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	PatrolingPath = CreateDefaultSubobject<USplineComponent>(TEXT("Spline Path"));
	PatrolingPath->SetupAttachment(GetRootComponent());

	PointTXT = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Template"));
	AllPoints.Empty();
	//MarkPoint();

}

// Called when the game starts or when spawned
void AArrowsPathAI::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AArrowsPathAI::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AArrowsPathAI::MarkPoint()
{
	for(int i =0;  i <= PatrolingPath->GetNumberOfSplinePoints(); i++)
	{

	//	UTextRenderComponent* PointText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("P")); 
		FTransform CompTransform;
		CompTransform.SetLocation(PatrolingPath->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World));

		UTextRenderComponent* PointText =  Cast<UTextRenderComponent>(AddComponent(FName("P"), true, CompTransform, PointTXT));

		if (PointText)
		{
			PointText->SetText("P" + FString::FromInt(i));
			PointText->SetRelativeLocation(PatrolingPath->GetScaleAtSplinePoint(i));
		}
		
	}
	
}

