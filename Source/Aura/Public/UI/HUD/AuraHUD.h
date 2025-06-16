// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/HUD.h"
#include "AuraHUD.generated.h"

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

// GameMode 초기화 이후 호출
DECLARE_MULTICAST_DELEGATE(FOnInitializeGameMode);

// PlayerController 초기화 이후 호출
DECLARE_MULTICAST_DELEGATE(FOnInitializePlayerController);

// GameMode에게 업그레이드 카드 요청
DECLARE_MULTICAST_DELEGATE_OneParam(FOnInitializeCards, APlayerController*);

// GameMode에게서 업그레이드 카드 받음
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnInitializeCardsReceived, FGameplayTag, FGameplayTag, FGameplayTag);

UCLASS()
class AURA_API AAuraHUD : public AHUD
{
	GENERATED_BODY()

public:
	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& WCParams);
	UAttributeMenuWidgetController* GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams);
	USpellMenuWidgetController* GetSpellMenuWidgetController(const FWidgetControllerParams& WCParams);
	UGameOverWidgetController* GetGameOverWidgetController(const FWidgetControllerParams& WCParams);
	USaveProgressWidgetController* GetSaveProgressWidgetController(const FWidgetControllerParams& WCParams);
	UMVVM_CardSelection* GetCardSelectionViewModel() { return CardSelectionViewModel; }
	
	void InitOverlay(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);

public:
	FOnInitializeGameMode OnInitializeGameModeDelegate;
	FOnInitializePlayerController OnInitializePlayerControllerDelegate;
	FOnInitializeCards InitializeCardsDelegate;
	FOnInitializeCardsReceived ReceivedCardsDelegate;
	
public:
	void CreateSaveProgressWidget();
	void RemoveSaveProgressWidget();

	UFUNCTION()
	void OnInitializePlayerController();
	
	void InitializeCardSelectionUI();
	void HandleRandomAbilityUpgrade(FGameplayTag UpgradeTag0, FGameplayTag UpgradeTag1, FGameplayTag UpgradeTag2);
	
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
	TObjectPtr<UGameOverWidgetController> GameOverWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameOverWidgetController> GameOverWidgetControllerClass;

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

};
