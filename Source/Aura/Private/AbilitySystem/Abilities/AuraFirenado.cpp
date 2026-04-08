// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraFirenado.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraFireTornado.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"

FString UAuraFirenado::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	const float CalculatedSpellRange = DefaultAbilityRange + (RangePerLevel * Level);
	
	return FString::Printf(TEXT(
		"<Title>화염 폭풍</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>해당 범위에 틱 당 </><Damage>%d</><Default>의 피해를 입히는 화염 기둥을 </><Num>%.1f초</><Default> 동안 소환합니다.</>\n<Small>최대 %.f의 거리에 있는 적을 추적하며, %.f 거리 내의 적에게 데미지를 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		CalculatedSpellRange,
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
	const float CalculatedSpellRange = DefaultAbilityRange + (RangePerLevel * Level);
	
	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>해당 범위에 틱 당 </><Damage>%d</><Default>의 피해를 입히는 화염 기둥을 </><Num>%.1f초</><Default> 동안 소환합니다.</>\n<Small>최대 %.f의 거리에 있는 적을 추적하며, %.f 거리 내의 적에게 데미지를 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		CalculatedSpellRange,
		ScaledDamage + MagicPowerDamage,
		SpawnTime,
		FollowRadius,
		DamageRadius
	);
}

void UAuraFirenado::CheckAbilityUpgrades()
{
	const auto& Tags = FAuraGameplayTags::Get();

	// 업그레이드 태그 검증
	// (1) 범위 증가 태그
	FGameplayTag IncreaseRange = Tags.Upgrades_Fire_FireNado_IncreaseRange;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseRange))
	{
		int Stacks = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncreaseRange);

		DamageRadius += 200.f * Stacks;
	}
}

void UAuraFirenado::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	StopAutoRun();
	
}

void UAuraFirenado::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	GetWorld()->GetTimerManager().ClearTimer(DestroyTimer);
}

void UAuraFirenado::CalculateRange()
{
	// 어빌리티 거리 계산
	AbilityRange = DefaultAbilityRange + (RangePerLevel * GetAbilityLevel());
}

AAuraFireTornado* UAuraFirenado::SpawnTornadoToLocation(const FVector& Location)
{
	HideMagicCircleAndRangeIndicator();
	StopAutoRun();
	
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Location);

	// 월드에 생성
	AAuraFireTornado* Tornado = GetWorld()->SpawnActorDeferred<AAuraFireTornado>(
		FireTornadoClass,
		SpawnTransform,
		GetOwningActorFromActorInfo(),
		Cast<APawn>(GetOwningActorFromActorInfo()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	Tornado->SetLifeSpan(SpawnTime);
	
	// 입력받은 Location이 0에 가까우면 마우스 위치에 소환
	if (Location.IsNearlyZero())
	{
		Tornado->SetActorLocation(MouseLocation);
	}
	else
	{
		Tornado->SetActorLocation(Location);
	}
	
	Tornado->SetDamageDeltaSecond(DamageDeltaSecond);
	Tornado->SetFollowRadius(FollowRadius);
	Tornado->SetDamageRadius(DamageRadius);
	
	Tornado->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Tornado->SetOwner(GetAvatarActorFromActorInfo());
	Tornado->OnDestroyed.AddDynamic(this, &UAuraFirenado::DestroyTornadoAndCommitCooldownEndAbility);

	Tornado->FinishSpawning(SpawnTransform);
	
	FireTornado = Tornado;
	K2_CommitAbilityCost();
	CommitAbilityCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);

	return Tornado;
}

void UAuraFirenado::DestroyTornadoAndCommitCooldownEndAbility(AActor* DestroyedActor)
{
	// 사운드 재생
	UGameplayStatics::PlaySoundAtLocation(this, DestroySound, DestroyedActor->GetActorLocation());

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}