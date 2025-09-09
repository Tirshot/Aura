// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ManaSiphon.h"

FString UManaSiphon::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const float EffectValue = CalculateManaDrainEfficiency(Level) * 100;
	return FString::Printf(TEXT(
		"<Title>마나 흡수</>\n<Small>레벨 </><Level>%d</>\n<Small>효과 </><Range>%.1f</><Small>퍼센트</>\n<Default>활성화 중일 때, 캐릭터가 입히는 데미지의 </><Num>%.1f</><Default>의 마나를 회복합니다.</>"),
		Level,
		EffectValue,
		EffectValue
	);
}

FString UManaSiphon::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const float EffectValue = CalculateManaDrainEfficiency(Level) * 100;

	if (Level <= MaxLevel)
	{
		return FString::Printf(TEXT(
		"<Title>다음 레벨:</>\n<Small>레벨 </><Level>%d</>\n<Small>효과 </><Range>%.1f</><Small>퍼센트</>\n<Default>활성화 중일 때, 캐릭터가 입히는 데미지의 </><Num>%.1f</><Default>의 마나를 회복합니다.</>"),
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

// ManaSiphon 계산은 AuraAttributeSet에서
float UManaSiphon::CalculateManaDrainEfficiency(const int32 Level) const
{
	return Level*ManaDrainEfficiencyPerDamage + DefaultManaDrainEfficiency;
}

float UManaSiphon::CalculateManaHealAmount(const int32 Level, const float InDamage) const
{
	return FMath::Max(CalculateManaDrainEfficiency(Level) * InDamage, 1.0f);
}
