// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ChainedLightning.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/ChainedLightningActor.h"
#include "Interaction/CombatInterface.h"


FString UChainedLightning::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	return FString::Printf(TEXT(
		"<Title>체인 라이트닝</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>반사 반경 </><Range>%.1f</>\n<Default>대상을 향해 </><Damage>%d</><Default>의 피해를 입히는 번개 줄기를 소환합니다.</>\n<Num>다른 타겟에게 최대 %i회</> 반사됩니다."),
		Level,
		ManaCost,
		Cooldown,
		ChainRadius,
		ScaledDamage + MagicPowerDamage,
		DefaultChain + Level
	);
}

FString UChainedLightning::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();

	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>반사 반경 </><Range>%.1f</>\n<Default>대상을 향해 </><Damage>%d</><Default>의 피해를 입히는 번개 줄기를 소환합니다.</>\n<Num>다른 타겟에게 최대 %i회</> 반사됩니다."),
		Level,
		ManaCost,
		Cooldown,
		ChainRadius,
		ScaledDamage + MagicPowerDamage,
		DefaultChain + Level
	);
}

void UChainedLightning::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	MaxChain = DefaultChain + GetAbilityLevel();
}

void UChainedLightning::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UChainedLightning::CheckAbilityUpgrades()
{
	const auto& Tags = FAuraGameplayTags::Get();

	// 업그레이드 태그 검증
	// (1) 추가 타겟 증가
	FGameplayTag IncreaseNum = FGameplayTag::RequestGameplayTag("Upgrades.Lightning.ChainedLightning.AdditionalTarget");
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseNum))
	{
		MaxChain += GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncreaseNum);
	}
	
	// (2) 추가 타겟에게 감소된 데미지를 입히지 않음
	FGameplayTag NoReduction = FGameplayTag::RequestGameplayTag("Upgrades.Lightning.ChainedLightning.NoReduction");
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), NoReduction))
	{
		DamageReductionRatio = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), NoReduction);
	}
	
	// (2) 추가 타겟에게 감소된 데미지를 입히지 않음
	FGameplayTag IncChainRadius = FGameplayTag::RequestGameplayTag("Upgrades.Lightning.ChainedLightning.IncreaseChainRadius");
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncChainRadius))
	{
		ChainRadius = DefaultChainRadius + (100.f * GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncChainRadius));
	}
}

void UChainedLightning::SpawnProjectile(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag,
	bool bOverridePitch, float PitchOverride)
{
	const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(
	GetAvatarActorFromActorInfo(),
	SocketTag);

	// 소켓에서 타겟까지의 벡터의 각도만 가져옴
	FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();

	// 높이 보정
	if (bOverridePitch)
	{
		Rotation.Pitch = PitchOverride;
	}

	FTransform SpawnTransform;
	SpawnTransform.SetLocation(SocketLocation);
	SpawnTransform.SetRotation(Rotation.Quaternion());

	AChainedLightningActor* Projectile = GetWorld()->SpawnActorDeferred<AChainedLightningActor>(
		ProjectileClass,
		SpawnTransform,
		GetOwningActorFromActorInfo(),
		Cast<APawn>(GetOwningActorFromActorInfo()),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	
	Projectile->SetOwner(GetAvatarActorFromActorInfo());
	Projectile->SetDamageReductionRatio(DamageReductionRatio);
	Projectile->SetChainRadius(ChainRadius);
	Projectile->SetMaxChain(MaxChain);
	Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Projectile->FinishSpawning(SpawnTransform);
}
