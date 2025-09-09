// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/HaloOfProtection.h"

FString UHaloOfProtection::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const float EffectValue = CalculateDamageReduce(Level) * 100;
	return FString::Printf(TEXT(
		"<Title>보호의 광휘</>\n<Small>레벨 </><Level>%d</>\n<Small>효과 </><Range>%.1f</><Small>퍼센트</>\n<Default>활성화 중일 때, 캐릭터가 입은 데미지의 </><Num>%.1f</><Default>를 상쇄하는 보호의 광휘를 소환합니다.</>"),
		Level,
		EffectValue,
		EffectValue
	);
}

FString UHaloOfProtection::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const float EffectValue = CalculateDamageReduce(Level) * 100;

	if (Level <= MaxLevel)
	{
		return FString::Printf(TEXT(
		   "<Title>다음 레벨:</>\n<Small>레벨 </><Level>%d</>\n<Small>효과 </><Range>%.1f</><Small>퍼센트</>\n<Default>활성화 중일 때, 캐릭터가 입은 데미지의 </><Num>%.1f</><Default>를 상쇄하는 보호의 광휘를 소환합니다.</>"),
		   Level,
		   EffectValue,
		   EffectValue
	   );
	}
	else
	{
		return FString::Printf(TEXT(
		   "<Default>어빌리티의 최대 레벨에 도달하였습니다.</>")
		);
	}
}

float UHaloOfProtection::CalculateDamageReduce(int32 Level)
{
	// 레벨에 따라 데미지 감소 비율 산정, 기본 20프로, 적용은 Exec_Calc에서
	return DefaultDamageReduceEfficiency + DamageReduceEfficiencyPerLevel * Level;
}
