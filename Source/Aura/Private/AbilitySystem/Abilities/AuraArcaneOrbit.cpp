// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraArcaneOrbit.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraArcaneMissile.h"

FString UAuraArcaneOrbit::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	return Super::GetDescription(Level, WorldContextObject);
}

FString UAuraArcaneOrbit::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	return Super::GetNextLevelDescription(Level, WorldContextObject);
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

		// 50프로 증가
		AbilityRange += (AbilityRange * 0.5f) * Stacks;
	}
}

TArray<AAuraArcaneMissile*> UAuraArcaneOrbit::SpawnArcaneMissiles()
{
	CommitAbilityCost(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
	
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

		// 월드에 파이어볼 생성
		AAuraArcaneMissile* ArcaneMissile = GetWorld()->SpawnActorDeferred<AAuraArcaneMissile>(
			ArcaneMissileClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			GetAvatarActorFromActorInfo()->GetInstigatorController()->GetPawn(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		// 클래스 디폴트의 데미지 값 적용
		ArcaneMissile->InitialAngle = Angle;
		ArcaneMissile->OrbitRadius = OrbitRadius;
		ArcaneMissile->OrbitSpeed = OrbitSpeed;
		ArcaneMissile->FollowRadius = AbilityRange;

		ArcaneMissile->SetOwnedAbility(this);
		
		ArcaneMissile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
		ArcaneMissile->ExplosionDamageParams = MakeDamageEffectParamsFromClassDefaults();
		ArcaneMissile->SetOwner(GetAvatarActorFromActorInfo());

		OutMissiles.Add(ArcaneMissile);

		ArcaneMissile->FinishSpawning(SpawnTransform);
	}
	Missiles = OutMissiles;
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
	AAuraArcaneMissile* Missile = Cast<AAuraArcaneMissile>(DestroyedMissile);
	Missiles.Remove(Missile);

	// 미사일이 모두 파괴되었으면 어빌리티 종료
	if (Missiles.IsEmpty())
	{
		CommitAbilityCooldown(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}
