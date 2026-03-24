// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/AuraMMCBase.h"

float UAuraMMCBase::GetBonusValue(const FGameplayEffectSpec& Spec, FGameplayTag BonusTag) const
{
	return Spec.GetSetByCallerMagnitude(BonusTag, false, 0.f);
}
