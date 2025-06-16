// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MVVM_CardSelection.generated.h"

class UMVVM_AbilityCard;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEnableSelectButton, bool, bEnable);

UCLASS()
class AURA_API UMVVM_CardSelection : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	void InitializeSlot();

public:
	UFUNCTION(BlueprintPure)
	UMVVM_AbilityCard* GetCardViewModelByIndex(int32 Index);

	int32 GetNumCards() { return AbilityCards.Num(); }
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMVVM_AbilityCard> AbilityCardViewModelClass;

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
