// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_BlockChance.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAttributeSet.h"

UMMC_BlockChance::UMMC_BlockChance()
{
	ArmorDef.AttributeToCapture = UAuraAttributeSet::GetArmorAttribute();
	ArmorDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	ArmorDef.bSnapshot = false;
	
	RelevantAttributesToCapture.Add(ArmorDef);
}

float UMMC_BlockChance::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// non-const
	float Armor = 0.f;
	GetCapturedAttributeMagnitude(ArmorDef, Spec, EvaluationParameters, Armor);

	Armor = FMath::Max<float>(Armor, 0.f);
	
	float BaseValue =  (Armor * 0.25f) + 4;
	float BonusValue = GetBonusValue(Spec, FAuraGameplayTags::Get().Attributes_Secondary_BlockChance);

	return BaseValue + BonusValue;
}
