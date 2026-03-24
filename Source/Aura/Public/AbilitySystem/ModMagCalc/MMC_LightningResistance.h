// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraMMCBase.h"
#include "MMC_LightningResistance.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMMC_LightningResistance : public UAuraMMCBase
{
	GENERATED_BODY()
	
public:
	UMMC_LightningResistance();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
private:
	FGameplayEffectAttributeCaptureDefinition ResilienceDef;
};
