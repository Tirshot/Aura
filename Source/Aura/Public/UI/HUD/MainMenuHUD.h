// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MainMenuHUD.generated.h"

class UAuraUserWidget;
struct FWidgetControllerParams;
class USettingsMenuWidgetController;
/**
 * 
 */
UCLASS()
class AURA_API AMainMenuHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	
	USettingsMenuWidgetController* GetSettingsMenuWidgetController();
	
private:
	UPROPERTY()
	TObjectPtr<UAuraUserWidget> OverlayWidget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAuraUserWidget> OverlayWidgetClass;
	
	UPROPERTY()
	TObjectPtr<USettingsMenuWidgetController> SettingsMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USettingsMenuWidgetController> SettingsMenuWidgetControllerClass;
};
