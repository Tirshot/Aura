// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/HUD.h"
#include "UI/ViewModel/MVVM_Inventory.h"
#include "UI/Widget/ItemToolTipWidget.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "AuraHUD.generated.h"

class UItemToolTipWidgetController;
class USettingsMenuWidgetController;
class UMVVM_DebugMenu;
class USpellUpgradesWidgetController;
struct FAuraAbilityUpgradeInfo;
class UMVVM_LoadScreen;
class ULoadScreenWidget;
class UMVVM_CardSelection;
class USaveProgressWidgetController;
class UGameOverWidgetController;
class UAuraUserWidget;
class UOverlayWidgetController;
class UAttributeMenuWidgetController;
class UAbilitySystemComponent;
class UAttributeSet;
struct FWidgetControllerParams;
class USpellMenuWidgetController;

UCLASS()
class AURA_API AAuraHUD : public AHUD
{
	GENERATED_BODY()

public:
	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams);
	USpellMenuWidgetController* GetSpellMenuWidgetController(const FWidgetControllerParams& WCParams);
	USpellUpgradesWidgetController* GetSpellUpgradesWidgetController(const FWidgetControllerParams& WCParams);
	UGameOverWidgetController* GetGameOverWidgetController(const FWidgetControllerParams& WCParams);
	USaveProgressWidgetController* GetSaveProgressWidgetController(const FWidgetControllerParams& WCParams);
	USettingsMenuWidgetController* GetSettingsMenuWidgetController(const FWidgetControllerParams& WCParams);
	UItemToolTipWidgetController* GetItemToolTipWidgetController(const FWidgetControllerParams& WCParams);
	UMVVM_CardSelection* GetCardSelectionViewModel() { return CardSelectionViewModel; }
	UMVVM_DebugMenu* GetDebugMenuViewModel() { return DebugMenuViewModel; }
	UMVVM_Inventory* GetInventoryViewModel();

	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	UFUNCTION()
	void ResetWidgetControllerAndViewModels();
	
protected:
	virtual void BeginPlay() override;
	
public:
	UFUNCTION()
	void OnPlayerStateCardsOninitialized(TArray<FAuraAbilityUpgradeInfo>& UpgradeInfos);
	
public:	
	void CreateSaveProgressWidget();
	void RemoveSaveProgressWidget();
	
	UFUNCTION()
	void HandleRandomAbilityUpgradeInfos(TArray<FAuraAbilityUpgradeInfo>& UpgradeInfos);

	UFUNCTION()
	void ShowOverlay();
	
	UFUNCTION()
	void HideOverlay();
	
	UFUNCTION()
	UAuraUserWidget* GetOverlayWidget() {return OverlayWidget;}
	
	UFUNCTION()
	UAuraUserWidget* GetSaveProgressWidget() {return SaveProgressWidget;}
	
private:
	UPROPERTY()
	TObjectPtr<UAuraUserWidget> OverlayWidget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAuraUserWidget> OverlayWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UAuraUserWidget> SaveProgressWidget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAuraUserWidget> SaveProgressWidgetClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USaveProgressWidgetController> SaveProgressWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<USaveProgressWidgetController> SaveProgressWidgetController;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;
	
	UPROPERTY()
	TObjectPtr<UAttributeMenuWidgetController> AttributeMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAttributeMenuWidgetController> AttributeMenuWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<USpellMenuWidgetController> SpellMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USpellMenuWidgetController> SpellMenuWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<USpellUpgradesWidgetController> SpellUpgradesWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USpellUpgradesWidgetController> SpellUpgradesWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UGameOverWidgetController> GameOverWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameOverWidgetController> GameOverWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<USettingsMenuWidgetController> SettingsMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USettingsMenuWidgetController> SettingsMenuWidgetControllerClass;
	
	UPROPERTY()
	TObjectPtr<UItemToolTipWidgetController> ItemToolTipWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UItemToolTipWidgetController> ItemToolTipWidgetControllerClass;

public:
	// MVVM
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> CardSelectionWidgetClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<ULoadScreenWidget> CardSelectionWidget;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMVVM_CardSelection> CardSelectionViewModelClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMVVM_CardSelection> CardSelectionViewModel;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMVVM_DebugMenu> DebugMenuViewModelClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMVVM_DebugMenu> DebugMenuViewModel;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMVVM_Inventory> InventoryMenuViewModelClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMVVM_Inventory> InventoryMenuViewModel;

};
