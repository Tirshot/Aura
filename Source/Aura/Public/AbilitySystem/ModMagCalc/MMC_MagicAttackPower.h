// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraMMCBase.h"
#include "MMC_MagicAttackPower.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMMC_MagicAttackPower : public UAuraMMCBase
{
	GENERATED_BODY()
	
public:
	UMMC_MagicAttackPower();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition IntelligenceDef;
};
