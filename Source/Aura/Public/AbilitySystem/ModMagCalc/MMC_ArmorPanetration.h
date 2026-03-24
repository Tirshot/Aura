// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraMMCBase.h"
#include "MMC_ArmorPanetration.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMMC_ArmorPanetration : public UAuraMMCBase
{
	GENERATED_BODY()
	
public:
	UMMC_ArmorPanetration();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
private:
	FGameplayEffectAttributeCaptureDefinition StrengthDef;
	
};
