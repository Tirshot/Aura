// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraPassiveAbility.h"
#include "ManaSiphon.generated.h"

UCLASS()
class AURA_API UManaSiphon : public UAuraPassiveAbility
{
	GENERATED_BODY()

public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;
	
	float CalculateManaDrainEfficiency(const int32 Level) const;
	float CalculateManaHealAmount(const int32 Level, const float InDamage) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category="ManaSiphon")
	float ManaDrainEfficiencyPerDamage = 0.05f;
	
	UPROPERTY(EditDefaultsOnly, Category="ManaSiphon")
	float DefaultManaDrainEfficiency = 0.15f;
};
