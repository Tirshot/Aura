// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_LightningResistance.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAttributeSet.h"

UMMC_LightningResistance::UMMC_LightningResistance()
{
	ResilienceDef.AttributeToCapture = UAuraAttributeSet::GetResilienceAttribute();
	ResilienceDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ResilienceDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(ResilienceDef);
}

float UMMC_LightningResistance::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
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

	float BaseValue = (Resilience * 0.2f) + 1;
	float BonusValue = GetBonusValue(Spec, FAuraGameplayTags::Get().Attributes_Primary_Resilience);

	return BaseValue + BonusValue;
}
