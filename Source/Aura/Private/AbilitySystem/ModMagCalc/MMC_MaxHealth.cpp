// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxHealth.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Interaction/CombatInterface.h"

UMMC_MaxHealth::UMMC_MaxHealth()
{
	// vigor �Ӽ� ĸ���ϱ�
	VigorDef.AttributeToCapture = UAuraAttributeSet::GetVigorAttribute();

	// �ҽ��� Ÿ���� ����(AuraCharacter)
	VigorDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;

	// ù ������ �Ӽ��� ĸ���� ���ΰ�, ���� ������ �Ӽ��� ĸ���� ���ΰ�
	VigorDef.bSnapshot = false;

	// ĸ���� �Ӽ����� Array�� �߰�
	RelevantAttributesToCapture.Add(VigorDef);
}

float UMMC_MaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// �ҽ��� ����� �±� ����
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// �Ӽ��� �� ��������
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// non-const
	float Vigor = 0.f;
	GetCapturedAttributeMagnitude(VigorDef, Spec, EvaluationParameters, Vigor);
	// Vigor�� Ÿ���� Ȱ�°��� �����Ե�

	// Ȱ�� ��� Ŭ����
	Vigor = FMath::Max<float>(Vigor, 0.f);

	// ������ ���� ����
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatInterface->GetPlayerLevel();

	// ����
	return 80.f + Vigor * 2.5f + 10.f * PlayerLevel;
}
