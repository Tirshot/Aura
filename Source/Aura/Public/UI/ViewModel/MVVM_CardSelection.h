// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MVVM_CardSelection.generated.h"

class ULoadScreenWidget;
class UMVVM_AbilityCard;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardSelectionViewModelInitialized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCardSelectionViewInitialized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnableSelectButton, bool, bEnable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRerollSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCloseSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeSelectedOnCard);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloseButtonEnableChanged, bool, bEnable);

UCLASS()
class AURA_API UMVVM_CardSelection : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	void InitializeSlot();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnCardSelectionViewModelInitialized OnCardSelectionViewModelInitialized;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnCardSelectionViewInitialized OnCardSelectionViewInitialized;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnUpgradeSelectedOnCard OnUpgradeSelectedOnCardDelegate;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnCloseSelected OnCloseSelectedDelegate;
	
public:
	UFUNCTION(BlueprintPure)
	UMVVM_AbilityCard* GetCardViewModelByIndex(int32 Index);

	int32 GetNumCards() { return AbilityCards.Num(); }
	
	FOnRerollSelected OnRerollSelectedDelegate;
	
	UFUNCTION(BlueprintCallable)
	void RerollButtonClicked();

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnCloseButtonEnableChanged OnCloseButtonEnableChangedDelegate;
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMVVM_AbilityCard> AbilityCardViewModelClass;

	UPROPERTY()
	ULoadScreenWidget* CardSelectionView;
	
private:
	UPROPERTY()
	TMap<int32, UMVVM_AbilityCard*> AbilityCards;

	UPROPERTY()
	TObjectPtr<UMVVM_AbilityCard> Card_0;
	
	UPROPERTY()
	TObjectPtr<UMVVM_AbilityCard> Card_1;
	
	UPROPERTY()
	TObjectPtr<UMVVM_AbilityCard> Card_2;
};
