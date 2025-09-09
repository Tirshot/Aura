// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraPassiveAbility.h"
#include "LifeSiphon.generated.h"

UCLASS()
class AURA_API ULifeSiphon : public UAuraPassiveAbility
{
	GENERATED_BODY()

public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;
	
	float CalculateLifeDrainEfficiency(const int32 Level) const;
	float CalculateLifeHealAmount(const int32 Level, const float InDamage) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category="LifeSiphon")
	float LifeDrainEfficiencyPerDamage = 0.05f;
	
	UPROPERTY(EditDefaultsOnly, Category="LifeSiphon")
	float DefaultLifeDrainEfficiency = 0.1f;
};
