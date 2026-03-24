// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_ManaRegeneration.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAttributeSet.h"

UMMC_ManaRegeneration::UMMC_ManaRegeneration()
{
	VigorDef.AttributeToCapture = UAuraAttributeSet::GetVigorAttribute();
	VigorDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	VigorDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(VigorDef);
}

float UMMC_ManaRegeneration::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// non-const
	float Vigor = 0.f;
	GetCapturedAttributeMagnitude(VigorDef, Spec, EvaluationParameters, Vigor);

	Vigor = FMath::Max<float>(Vigor, 0.f);

	float BaseValue = (Vigor * 0.5f) + 1;
	float BonusValue = GetBonusValue(Spec, FAuraGameplayTags::Get().Attributes_Primary_Vigor);

	return BaseValue + BonusValue;
}
