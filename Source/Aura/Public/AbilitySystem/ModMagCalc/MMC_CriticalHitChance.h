// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraMMCBase.h"
#include "MMC_CriticalHitChance.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMMC_CriticalHitChance : public UAuraMMCBase
{
	GENERATED_BODY()
	
public:
	UMMC_CriticalHitChance();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
private:
	FGameplayEffectAttributeCaptureDefinition ArmorPenetrationDef;
};
