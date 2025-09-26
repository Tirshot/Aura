// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraArcaneAreaAbility.h"

#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraArcaneArea.h"

FString UAuraArcaneAreaAbility::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	const int32 SlowDownPercent = SlowDownRatio * 100;

	if (bTakeDamage)
	{
		// 데미지를 입히는 업그레이드 선택 시 설명
		return FString::Printf(TEXT(
			"<Title>아케인 영역</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>스펠 범위 내에 매 틱마다 </><Damage>%d</><Default>의 피해를 입히는 </><Num>%.1f</><Default> 크기의 아케인 영역을 </><Num>%.1f초</><Default> 동안 소환하여 적을 중앙으로 끌어당깁니다.</>\n<Default>적의 이동속도를 매 </><Num>%.1f초</><Default>마다 </><Num>%d퍼센트 </><Default>감소시킵니다.</>\n<Small>이동속도 감소 효과를 5스택 이상 보유한 적은 기절합니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			AbilityRange,
			ScaledDamage + MagicPowerDamage,
			SlowRadius,
			LifeSpan,
			ApplyEffectPeriod,
			SlowDownPercent
		);
	}
	
	// 일반적인 설명
	return FString::Printf(TEXT(
		"<Title>아케인 영역</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>스펠 범위 내에</><Num>%.1f</><Default> 크기의 아케인 영역을 </><Num>%.1f초</><Default> 동안 소환하여, </>\n<Default>적의 이동속도를 매 </><Num>%.1f초</><Default>마다 </><Num>%d퍼센트 </><Default>감소시킵니다.</>\n<Small>이동속도 감소 효과를 5스택 이상 보유한 적은 기절합니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		AbilityRange,
		SlowRadius,
		LifeSpan,
		ApplyEffectPeriod,
		SlowDownPercent
	);
}

FString UAuraArcaneAreaAbility::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	const int32 SlowDownPercent = SlowDownRatio * 100;

	if (bTakeDamage)
	{
		// 데미지를 입히는 업그레이드 선택 시 설명
		return FString::Printf(TEXT(
			"<Title>다음 레벨 :</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>스펠 범위 내에 매 틱마다 </><Damage>%d</><Default>의 피해를 입히는 </><Num>%.1f</><Default> 크기의 아케인 영역을 </><Num>%.1f초</><Default> 동안 소환하여 적을 중앙으로 끌어당깁니다.</>\n<Default>적의 이동속도를 매 </><Num>%.1f초</><Default>마다 </><Num>%d퍼센트 </><Default>감소시킵니다.</>\n<Small>이동속도 감소 효과를 5스택 이상 보유한 적은 기절합니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			AbilityRange,
			ScaledDamage + MagicPowerDamage,
			SlowRadius,
			LifeSpan,
			ApplyEffectPeriod,
			SlowDownPercent
		);
	}
	
	// 일반적인 설명
	return FString::Printf(TEXT(
		"<Title>다음 레벨 :</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>스펠 범위 내에</><Num>%.1f</><Default> 크기의 아케인 영역을 </><Num>%.1f초</><Default> 동안 소환하여, </>\n<Default>적의 이동속도를 매 </><Num>%.1f초</><Default>마다 </><Num>%d퍼센트 </><Default>감소시킵니다.</>\n<Small>이동속도 감소 효과를 5스택 이상 보유한 적은 기절합니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		AbilityRange,
		SlowRadius,
		LifeSpan,
		ApplyEffectPeriod,
		SlowDownPercent
	);
}

void UAuraArcaneAreaAbility::CheckAbilityUpgrades()
{
	const auto& Tags = FAuraGameplayTags::Get();

	// 업그레이드 태그 검증
	// (1) 공격형으로 전환
	FGameplayTag TakeDamage = FGameplayTag::RequestGameplayTag("Upgrades.Arcane.ArcaneArea.TakeDamage");
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), TakeDamage))
	{
		bTakeDamage = true;
	}

	// (2) 소환 범위 증가
	FGameplayTag IncreaseRange = FAuraGameplayTags::Get().Upgrades_Arcane_ArcaneArea_IncreaseRange;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseRange))
	{
		AbilityRange += AbilityRange * 0.25f;
	}

	FGameplayTag IncreaseSize = FAuraGameplayTags::Get().Upgrades_Arcane_ArcaneArea_IncreaseSize;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseSize))
	{
		SlowRadius += SlowRadius * 0.25f;
	}
}

void UAuraArcaneAreaAbility::SpawnArcaneArea(const FVector& Location)
{
	const FRotator& Rotation = GetAvatarActorFromActorInfo()->GetActorRotation();

	FTransform AreaTransform(Rotation, Location);
	
	ArcaneArea = GetWorld()->SpawnActorDeferred<AAuraArcaneArea>(
		ArcaneAreaClass,
		AreaTransform,
		GetAvatarActorFromActorInfo(),
		GetAvatarActorFromActorInfo()->GetInstigator());

	// 이펙트 클래스 설정
	ArcaneArea->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	ArcaneArea->SlowDownEffectClass = SlowDownEffectClass;
	ArcaneArea->SlowDownDecayEffectClass = SlowDownDecayEffectClass;
	ArcaneArea->OnDestroyed.AddDynamic(this, &UAuraArcaneAreaAbility::OnArcaneAreaDestroyed);

	// 멤버 변수 설정
	ArcaneArea->LifeSpan = LifeSpan;
	ArcaneArea->SetSlowRadius(SlowRadius);
	ArcaneArea->SetSlowSpeedRatio(SlowDownRatio);
	ArcaneArea->SetApplyEffectPeriod(ApplyEffectPeriod);
	ArcaneArea->SetTakeDamage(bTakeDamage);

	ArcaneArea->FinishSpawning(AreaTransform);
}

void UAuraArcaneAreaAbility::OnArcaneAreaDestroyed(AActor* DestroyedActor)
{
	K2_EndAbility();
}