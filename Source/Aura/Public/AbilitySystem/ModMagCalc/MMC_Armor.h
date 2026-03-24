// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraMMCBase.h"
#include "MMC_Armor.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMMC_Armor : public UAuraMMCBase
{
	GENERATED_BODY()
	
public:
	UMMC_Armor();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
	
private:
	FGameplayEffectAttributeCaptureDefinition ResilienceDef;
	
};
