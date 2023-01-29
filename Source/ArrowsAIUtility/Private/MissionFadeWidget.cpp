// Fill out your copyright notice in the Description page of Project Settings.


#include "MissionFadeWidget.h"

UMissionFadeWidget::UMissionFadeWidget(const FObjectInitializer& ObjectInitializer) :  UUserWidget(ObjectInitializer)
{

}

void UMissionFadeWidget::NativeConstruct()
{
	Super::NativeConstruct();

	//getting the fade animation
	FProperty* prop = GetClass()->PropertyLink;

	while (prop)
	{

		if (prop->GetClass() == FObjectProperty::StaticClass())
		{
			FObjectProperty* ObjProp = CastField<FObjectProperty>(prop);

			if (ObjProp->PropertyClass == UWidgetAnimation::StaticClass())
			{
				UObject* Obj = ObjProp->GetObjectPropertyValue_InContainer(this);
				UWidgetAnimation* WidgetAnimation = Cast<UWidgetAnimation>(Obj);

				if (WidgetAnimation)
				{
					FadeAnimation = WidgetAnimation;
				}
			}
		}

		prop = prop->PropertyLinkNext;
	}
}

void UMissionFadeWidget::PlayFadeAnimatoin(TEnumAsByte<EUMGSequencePlayMode::Type> FadeMode, float Rate)
{
	if (FadeAnimation)
	{
		UUserWidget::PlayAnimation(FadeAnimation, 0.0f, 1, FadeMode, Rate, false);
	}
}