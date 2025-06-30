// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraFirenado.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraFireTornado.h"

FString UAuraFirenado::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	return FString::Printf(TEXT(
		"<Title>화염 폭풍</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>해당 범위에 총 </><Damage>%d</><Default>의 피해를 입히는 화염 기둥을 </><Num>%.1f초</><Default> 동안 소환합니다.</>\n<Small>최대 %.f의 거리에 있는 적을 추적하며, %.f 거리 내의 적에게 데미지를 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		ScaledDamage + MagicPowerDamage,
		SpawnTime,
		FollowRadius,
		DamageRadius
	);
}

FString UAuraFirenado::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>해당 범위에 총 </><Damage>%d</><Default>의 피해를 입히는 화염 기둥을 </><Num>%.1f초</><Default> 동안 소환합니다.</>\n<Small>최대 %.f의 거리에 있는 적을 추적하며, %.f 거리 내의 적에게 데미지를 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		ScaledDamage + MagicPowerDamage,
		SpawnTime,
		FollowRadius,
		DamageRadius
	);
}

AAuraFireTornado* UAuraFirenado::SpawnTornado()
{
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Location);

	// 월드에 파이어볼 생성
	AAuraFireTornado* Tornado = GetWorld()->SpawnActorDeferred<AAuraFireTornado>(
		FireTornadoClass,
		SpawnTransform,
		GetOwningActorFromActorInfo(),
		CurrentActorInfo->PlayerController->GetPawn(),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	Tornado->SetDamageDeltaSecond(DamageDeltaSecond);
	Tornado->SetFollowRadius(FollowRadius);
	Tornado->SetDamageRadius(DamageRadius);
	
	Tornado->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Tornado->SetOwner(GetAvatarActorFromActorInfo());

	Tornado->FinishSpawning(SpawnTransform);

	return Tornado;
}

AAuraFireTornado* UAuraFirenado::SpawnTornadoToLocation(const FVector& Location)
{
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Location);

	// 월드에 파이어볼 생성
	AAuraFireTornado* Tornado = GetWorld()->SpawnActorDeferred<AAuraFireTornado>(
		FireTornadoClass,
		SpawnTransform,
		GetOwningActorFromActorInfo(),
		CurrentActorInfo->PlayerController->GetPawn(),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	Tornado->SetDamageDeltaSecond(DamageDeltaSecond);
	Tornado->SetFollowRadius(FollowRadius);
	Tornado->SetDamageRadius(DamageRadius);
	
	Tornado->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Tornado->SetOwner(GetAvatarActorFromActorInfo());

	Tornado->FinishSpawning(SpawnTransform);

	return Tornado;
}
