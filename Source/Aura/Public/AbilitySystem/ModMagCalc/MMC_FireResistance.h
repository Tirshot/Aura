// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraMMCBase.h"
#include "MMC_FireResistance.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMMC_FireResistance : public UAuraMMCBase
{
	GENERATED_BODY()
	
public:
	UMMC_FireResistance();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
private:
	FGameplayEffectAttributeCaptureDefinition ResilienceDef;
};
