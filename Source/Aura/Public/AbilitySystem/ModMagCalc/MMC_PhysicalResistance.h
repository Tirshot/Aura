// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraMMCBase.h"
#include "MMC_PhysicalResistance.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMMC_PhysicalResistance : public UAuraMMCBase
{
	GENERATED_BODY()
		
public:
	UMMC_PhysicalResistance();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
private:
	FGameplayEffectAttributeCaptureDefinition ResilienceDef;
};
