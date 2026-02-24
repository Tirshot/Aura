// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxMana.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Interaction/CombatInterface.h"

UMMC_MaxMana::UMMC_MaxMana()
{
	IntelligenceDef.AttributeToCapture = UAuraAttributeSet::GetIntelligenceAttribute();
	IntelligenceDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	IntelligenceDef.bSnapshot = false;

	RelevantAttributesToCapture.Add(IntelligenceDef);
}

float UMMC_MaxMana::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// �±�
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	//
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	// �Ӽ� ĸ��
	float Intelligence = 0.f;
	GetCapturedAttributeMagnitude(IntelligenceDef, Spec, EvaluateParameters, Intelligence);

	// �Ӽ� Ŭ����
	Intelligence = FMath::Max(Intelligence, 0.f);

	// ���� ��������
	int32 PlayerLevel = 1;

	if (FGameplayEffectContext* GEContext = Spec.GetContext().Get())
	{
		if (AActor* AvatarActor = Cast<AActor>(GEContext->GetSourceObject()))
		{
			if (AvatarActor->Implements<UCombatInterface>())
			{
				PlayerLevel = ICombatInterface::Execute_GetCharacterLevel(AvatarActor);
			}
		}
	}

	return 50 + 2.5f * Intelligence + 15 * PlayerLevel;
}
