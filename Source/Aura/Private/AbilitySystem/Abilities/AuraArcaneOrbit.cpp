// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraArcaneOrbit.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "GameplayCueManager.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraArcaneMissile.h"

FString UAuraArcaneOrbit::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();

	const float RealDamage = ScaledDamage + MagicPowerDamage;
	
	return FString::Printf(TEXT(
		"<Title>아케인 궤도</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>캐릭터 주위를 </><Num>%.1f</><Default>초 동안 회전하는 아케인 미사일을 </><Num>%d</><Default>개 소환합니다.</>\n<Default>일정 시간 이후, 각 미사일은 사거리 내의 적을 자동으로 공격하여 </><Damage>%.1f</><Default>의 피해를 입힙니다.</>\n<Small>타겟팅된 적이 사망하면 다시 궤도로 돌아옵니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		AbilityRange,
		MissileLifeSpan,
		NumMissiles,
		RealDamage
	);
}

FString UAuraArcaneOrbit::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();

	const float RealDamage = ScaledDamage + MagicPowerDamage;
	
	return FString::Printf(TEXT(
		"<Title>다음 레벨:</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>캐릭터 주위를 </><Num>%.1f</><Default>초 동안 회전하는 아케인 미사일을 </><Num>%d</><Default>개 소환합니다.</>\n<Default>일정 시간 이후, 각 미사일은 사거리 내의 적을 자동으로 공격하여 </><Damage>%.1f</><Default>의 피해를 입힙니다.</>\n<Small>타겟팅된 적이 사망하면 다시 궤도로 돌아옵니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		AbilityRange,
		MissileLifeSpan,
		NumMissiles,
		RealDamage
	);
}

void UAuraArcaneOrbit::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                       const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                       const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CheckAbilityUpgrades();
	
	SpawnArcaneMissiles();
}

void UAuraArcaneOrbit::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Missiles.Empty();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraArcaneOrbit::CheckAbilityUpgrades()
{
	const auto& Tags = FAuraGameplayTags::Get();

	// 업그레이드 태그 검증
	// (1) 범위 증가 태그
	FGameplayTag IncreaseRange = Tags.Upgrades_Arcane_ArcaneOrbit_IncreaseRange;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseRange))
	{
		int Stacks = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncreaseRange);

		// 25프로 증가
		AbilityRange += (AbilityRange * 0.25f) * Stacks;
	}

	// (2) 갯수 증가 태그
	FGameplayTag IncreaseNum = Tags.Upgrades_Arcane_ArcaneOrbit_IncreaseNum;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseNum))
	{
		int Stacks = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncreaseNum);

		// 스택 수 만큼 증가
		NumMissiles += Stacks;
	}
}

TArray<AAuraArcaneMissile*> UAuraArcaneOrbit::SpawnArcaneMissiles()
{
	if (!GetActorInfo().IsNetAuthority())
	{
		return TArray<AAuraArcaneMissile*>();
	}
	
	FGameplayCueParameters Params;
	Params.NormalizedMagnitude = OrbitRadius / 100.f;
	Params.Normal = GetAvatarActorFromActorInfo()->GetActorLocation();
	Params.TargetAttachComponent = GetAvatarActorFromActorInfo()->GetRootComponent();
	
	// 복제 되는 게임플레이 큐
	K2_ExecuteGameplayCueWithParams(
		FGameplayTag::RequestGameplayTag("GameplayCue.Arcane.Spawn"), Params);
	
	CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	CommitAbilityCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false);
	
	TArray<AAuraArcaneMissile*> OutMissiles;

	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();

	// 균일하게 회전하는 로테이터 가져오기
	TArray<FRotator> Rotators = UAuraAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, 360.f, NumMissiles);

	for (int32 i = 0; i < Rotators.Num(); ++i)
	{
		FVector Direction = Rotators[i].Vector();
		
		float Angle = FMath::DegreesToRadians((360.f / NumMissiles) * i);
		
		FTransform SpawnTransform;

		SpawnTransform.SetLocation(Location + Direction * OrbitRadius);
		SpawnTransform.SetRotation(Rotators[i].Quaternion());

		// 월드에 미사일 생성
		AAuraArcaneMissile* ArcaneMissile = GetWorld()->SpawnActorDeferred<AAuraArcaneMissile>(
			ArcaneMissileClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			GetAvatarActorFromActorInfo()->GetInstigatorController()->GetPawn(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		// 파라미터 전달
		ArcaneMissile->InitialAngle = Angle;
		ArcaneMissile->OrbitRadius = OrbitRadius;
		ArcaneMissile->OrbitSpeed = OrbitSpeed;
		ArcaneMissile->FollowRadius = AbilityRange;
		ArcaneMissile->InitialDelayDuration = InitialDelayDuration;

		// 소유 어빌리티 지정
		ArcaneMissile->SetOwnedAbility(this);
		
		// 클래스 디폴트의 데미지 값 적용
		ArcaneMissile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
		ArcaneMissile->ExplosionDamageParams = MakeDamageEffectParamsFromClassDefaults();
		ArcaneMissile->SetOwner(GetAvatarActorFromActorInfo());

		OutMissiles.Add(ArcaneMissile);

		ArcaneMissile->FinishSpawning(SpawnTransform);
		ArcaneMissile->SetLifeSpan(MissileLifeSpan);
	}
	Missiles = OutMissiles;
	
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	return OutMissiles;
}

void UAuraArcaneOrbit::DestroyAllMissiles()
{
	for (auto& Missile : Missiles)
	{
		Missile->Destroy();
	}
	
	Missiles.Empty();
}

void UAuraArcaneOrbit::MissileDestroyed(AActor* DestroyedMissile)
{
	if (!IsValid(DestroyedMissile))
	{
		K2_EndAbility();
		return;
	}
	
	if (AAuraArcaneMissile* Missile = Cast<AAuraArcaneMissile>(DestroyedMissile))
		Missiles.Remove(Missile);
}
