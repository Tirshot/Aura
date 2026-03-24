// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_ArcaneResistance.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAttributeSet.h"

UMMC_ArcaneResistance::UMMC_ArcaneResistance()
{
	ResilienceDef.AttributeToCapture = UAuraAttributeSet::GetResilienceAttribute();
	ResilienceDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ResilienceDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(ResilienceDef);
}

float UMMC_ArcaneResistance::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// non-const
	float Resilience = 0.f;
	GetCapturedAttributeMagnitude(ResilienceDef, Spec, EvaluationParameters, Resilience);

	Resilience = FMath::Max<float>(Resilience, 0.f);
	
	float BaseValue = (Resilience * 0.5f) + 3;
	float BonusValue = GetBonusValue(Spec, FAuraGameplayTags::Get().Attributes_Resistance_Arcane);

	return BaseValue + BonusValue;
}
