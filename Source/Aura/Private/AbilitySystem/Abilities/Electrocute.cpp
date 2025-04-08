// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Electrocute.h"

FString UElectrocute::GetDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	if (Level == 1)
	{
		return FString::Printf(TEXT(
			"<Title>감전사</>\n<Small>레벨 </><Level>%d</>\n<Small>매 초당 마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>해당 위치에 매 틱마다 </><Damage>%d</><Default>의 피해를 입히는 빔을 방출하여 일정 확률로 적을 기절시킵니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			ScaledDamage
		);
	}
	else
	{
		return FString::Printf(TEXT(
			"<Title>감전사</>\n<Small>레벨 </><Level>%d</>\n<Small>매 초당 마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>해당 위치에 매 틱마다 </><Damage>%d</><Default>의 피해를 입히는 빔을 방출하고, 주위의 적 %d명을 추가로 공격합니다. 일정 확률로 적을 기절시킵니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			ScaledDamage,
			Level - 1
		);
	}
}

FString UElectrocute::GetNextLevelDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Default>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>매 초당 마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>해당 위치에 매 틱마다 </><Damage>%d</><Default>의 피해를 입히는 빔을 방출하고, 주위의 적 %d명을 추가로 공격합니다. 일정 확률로 적을 기절시킵니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		ScaledDamage,
		Level - 1
		);
}