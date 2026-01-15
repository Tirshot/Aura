// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/MainMenuHUD.h"

#include "UI/WidgetController/SettingsMenuWidgetController.h"

void AMainMenuHUD::BeginPlay()
{
	Super::BeginPlay();
	
	GetSettingsMenuWidgetController();
}

USettingsMenuWidgetController* AMainMenuHUD::GetSettingsMenuWidgetController()
{
	if (SettingsMenuWidgetController == nullptr)
	{   // 없으면 생성
		SettingsMenuWidgetController = NewObject<USettingsMenuWidgetController>(this, SettingsMenuWidgetControllerClass);
		SettingsMenuWidgetController->BindCallbacksToDependencies();
	}

	return SettingsMenuWidgetController;
}
