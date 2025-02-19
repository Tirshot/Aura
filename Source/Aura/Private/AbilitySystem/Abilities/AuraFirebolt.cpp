
#include "AbilitySystem/Abilities/AuraFirebolt.h"
#include "Aura/Public/AuraGameplayTags.h"

FString UAuraFirebolt::GetDescription(int32 Level)
{
	const int32 Damage = GetDamageByDamageType(Level, FAuraGameplayTags::Get().Damage_Fire);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	if (Level == 1)
	{
		return FString::Printf(TEXT(
			"<Title>파이어볼트</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			Damage
			);
	}
	else
	{
		return FString::Printf(TEXT(
			"<Title>파이어볼트</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 %d개 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			FMath::Min(NumProjectiles, Level),
			Damage
		);
	}
}

FString UAuraFirebolt::GetNextLevelDescription(int32 Level)
{
	const int32 Damage = DamageTypes[FAuraGameplayTags::Get().Damage_Fire].GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 %d개 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		FMath::Min(NumProjectiles, Level),
		Damage
	);
}
