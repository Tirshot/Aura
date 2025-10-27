// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "SpellUpgradesWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReceivedUpgradeTags, FString, UpgradeName, int32, Stack);

UCLASS(BlueprintType, Blueprintable)
class AURA_API USpellUpgradesWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Data")
	TObjectPtr<UAbilityUpgradeInfo> AbilityUpgradeInfo;

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly)
	TMap<FString, int32> UpgradeInfo;
};
