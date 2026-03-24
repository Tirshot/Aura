// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_ArmorPanetration.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAttributeSet.h"

UMMC_ArmorPanetration::UMMC_ArmorPanetration()
{
	StrengthDef.AttributeToCapture = UAuraAttributeSet::GetStrengthAttribute();
	StrengthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	StrengthDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(StrengthDef);
}

float UMMC_ArmorPanetration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// non-const
	float Strength = 0.f;
	GetCapturedAttributeMagnitude(StrengthDef, Spec, EvaluationParameters, Strength);

	Strength = FMath::Max<float>(Strength, 0.f);
	
	float BaseValue =  (Strength * 0.5f + 1) + 16;
	float BonusValue = GetBonusValue(Spec, FAuraGameplayTags::Get().Attributes_Secondary_ArmorPenetration);

	return BaseValue + BonusValue;
}
