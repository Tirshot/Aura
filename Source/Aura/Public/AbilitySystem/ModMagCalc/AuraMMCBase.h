// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "AuraMMCBase.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraMMCBase : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
protected:
	float GetBonusValue(const FGameplayEffectSpec& Spec, FGameplayTag BonusTag) const;
};
