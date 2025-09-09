// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraPassiveAbility.h"
#include "HaloOfProtection.generated.h"

UCLASS()
class AURA_API UHaloOfProtection : public UAuraPassiveAbility
{
	GENERATED_BODY()

public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;
	
	float CalculateDamageReduce(int32 Level);
protected:
	UPROPERTY(EditDefaultsOnly, Category="HaloOfProtection")
	float DamageReduceEfficiencyPerLevel = 0.025f;
	
	UPROPERTY(EditDefaultsOnly, Category="HaloOfProtection")
	float DefaultDamageReduceEfficiency = 0.2f;
};
