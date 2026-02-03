// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/AuraOverlayWidget.h"

#include "UI/Widget/AuraCenterDescriptionWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"

void UAuraOverlayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (UOverlayWidgetController* OverlayWC = Cast<UOverlayWidgetController>(WidgetController))
	{
		OverlayWC->OnCenterDescriptionRemoved.AddDynamic(this, &UAuraOverlayWidget::RemoveCenterDescriptionText);
	}
}

void UAuraOverlayWidget::ClearKeyboardFocus()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().ClearKeyboardFocus(EFocusCause::Cleared);
		FSlateApplication::Get().SetAllUserFocusToGameViewport();
	}
}

void UAuraOverlayWidget::RemoveCenterDescriptionText()
{
	WBP_CenterTutorialDescription->TextBlock->SetText(FText());
}
