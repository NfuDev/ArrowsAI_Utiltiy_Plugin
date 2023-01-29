// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "MissionFadeWidget.generated.h"

/**
 * 
 */
UCLASS()
class ARROWSAIUTILITY_API UMissionFadeWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	UMissionFadeWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
	UBorder* FadeBorder;

	UWidgetAnimation* FadeAnimation;

	void PlayFadeAnimatoin(TEnumAsByte<EUMGSequencePlayMode::Type> FadeMode);
};
