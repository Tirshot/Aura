// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LoadScreenHUD.generated.h"

class USettingsMenuWidgetController;
class ULoadScreenWidget;
class UMVVM_LoadScreen;

UCLASS()
class AURA_API ALoadScreenHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	USettingsMenuWidgetController* GetSettingsMenuWidgetController();
	
public:
	// MVVM - (View)
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> LoadScreenWidgetClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULoadScreenWidget> LoadScreenWidget;

	// MVVM - (ViewModel)
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMVVM_LoadScreen> LoadScreenViewModelClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMVVM_LoadScreen> LoadScreenViewModel;
	
	UPROPERTY()
	TObjectPtr<USettingsMenuWidgetController> SettingsMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USettingsMenuWidgetController> SettingsMenuWidgetControllerClass;
	
protected:
	virtual void BeginPlay() override;
};
