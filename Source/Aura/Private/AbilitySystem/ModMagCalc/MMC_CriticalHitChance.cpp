// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_CriticalHitChance.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAttributeSet.h"

UMMC_CriticalHitChance::UMMC_CriticalHitChance()
{
	ArmorPenetrationDef.AttributeToCapture = UAuraAttributeSet::GetArmorPenetrationAttribute();
	ArmorPenetrationDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ArmorPenetrationDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(ArmorPenetrationDef);
}

float UMMC_CriticalHitChance::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// non-const
	float ArmorPenetration = 0.f;
	GetCapturedAttributeMagnitude(ArmorPenetrationDef, Spec, EvaluationParameters, ArmorPenetration);

	ArmorPenetration = FMath::Max<float>(ArmorPenetration, 0.f);

	float BaseValue = (ArmorPenetration * 0.5f) + 10;
	float BonusValue = GetBonusValue(Spec, FAuraGameplayTags::Get().Attributes_Secondary_ArmorPenetration);

	return BaseValue + BonusValue;
}
