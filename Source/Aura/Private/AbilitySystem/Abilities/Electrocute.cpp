// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Electrocute.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

UElectrocute::UElectrocute()
{
	SpellType = ESpellType::Targeting;
}

FString UElectrocute::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	if (Level == 1)
	{
		return FString::Printf(TEXT(
			"<Title>감전사</>\n<Small>레벨 </><Level>%d</>\n<Small>매 초당 마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>해당 위치에 매 틱마다 </><Damage>%d</><Default>의 피해를 입히는 빔을 방출하여 일정 확률로 적을 기절시킵니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			ScaledDamage + MagicPowerDamage
		);
	}
	else
	{
		return FString::Printf(TEXT(
			"<Title>감전사</>\n<Small>레벨 </><Level>%d</>\n<Small>매 초당 마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>해당 위치에 매 틱마다 </><Damage>%d</><Default>의 피해를 입히는 빔을 방출하고, 주위의 적 %d명을 추가로 공격합니다. 일정 확률로 적을 기절시킵니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			ScaledDamage + MagicPowerDamage,
			Level - 1
		);
	}
}

FString UElectrocute::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	return FString::Printf(TEXT(
		"<Default>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>매 초당 마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>해당 위치에 매 틱마다 </><Damage>%d</><Default>의 피해를 입히는 빔을 방출하고, 주위의 적 %d명을 추가로 공격합니다. 일정 확률로 적을 기절시킵니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		ScaledDamage + MagicPowerDamage,
		Level - 1
		);
}