// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ArcaneShards.h"

FString UArcaneShards::GetDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Title>아케인 파편</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>지정한 범위 내에 </><Damage>%d</><Default>의 피해를 입히는 아케인 파편 기둥을 %d개 소환합니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		ScaledDamage,
		Level
	);
}

FString UArcaneShards::GetNextLevelDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>지정한 범위 내에 </><Damage>%d</><Default>의 피해를 입히는 아케인 파편 기둥을 %d개 소환합니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		ScaledDamage,
		Level
	);
}
