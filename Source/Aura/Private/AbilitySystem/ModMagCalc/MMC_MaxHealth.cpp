// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ModMagCalc/MMC_MaxHealth.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Interaction/CombatInterface.h"

UMMC_MaxHealth::UMMC_MaxHealth()
{
	// vigor 속성 캡쳐하기
	VigorDef.AttributeToCapture = UAuraAttributeSet::GetVigorAttribute();

	// 소스와 타겟이 동일(AuraCharacter)
	VigorDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;

	// 첫 시점의 속성을 캡쳐할 것인가, 적용 순간의 속성을 캡쳐할 것인가
	VigorDef.bSnapshot = false;

	// 캡쳐할 속성들을 Array에 추가
	RelevantAttributesToCapture.Add(VigorDef);
}

float UMMC_MaxHealth::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	// 소스와 대상의 태그 수집
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	// 속성의 값 가져오기
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	// non-const
	float Vigor = 0.f;
	GetCapturedAttributeMagnitude(VigorDef, Spec, EvaluationParameters, Vigor);
	// Vigor는 타겟의 활력값을 가지게됨

	// 활력 양수 클램핑
	Vigor = FMath::Max<float>(Vigor, 0.f);

	// 레벨에 따라 증가
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(Spec.GetContext().GetSourceObject());
	const int32 PlayerLevel = CombatInterface->GetPlayerLevel();

	// 공식
	return 80.f + Vigor * 2.5f + 10.f * PlayerLevel;
}
