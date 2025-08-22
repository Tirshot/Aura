// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraSpawnElectroSphere.h"

#include "AuraGameplayTags.h"
#include "FileCache.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraElectroSphere.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interaction/CombatInterface.h"

FString UAuraSpawnElectroSphere::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	return Super::GetDescription(Level, WorldContextObject);
}

FString UAuraSpawnElectroSphere::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	return Super::GetNextLevelDescription(Level, WorldContextObject);
}

void UAuraSpawnElectroSphere::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAuraSpawnElectroSphere::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraSpawnElectroSphere::CheckAbilityUpgrades()
{
	const auto& Tags = FAuraGameplayTags::Get();

	// 업그레이드 태그 검증
	// (1) 추적 범위 증가 
	FGameplayTag IncreaseRange = Tags.Upgrades_Lightning_SpawnElectroSphere_IncreaseTraceRange;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseRange))
	{
		int Stacks = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncreaseRange);
	
		TraceRadius += 100.f * Stacks;
	}

	// (2) 이동속도 감소
	FGameplayTag DecreaseSpeed = Tags.Upgrades_Lightning_SpawnElectroSphere_DecreaseMovementSpeed;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), DecreaseSpeed))
	{
		int Stacks = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), DecreaseSpeed);
	
		ElectroSphere->AddMovementSpeed(-250.f * Stacks);
	}

	// (3) 가까운 대상 유도
	FGameplayTag HomingNearestTarget = Tags.Upgrades_Lightning_SpawnElectroSphere_HomingNearestTarget;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), HomingNearestTarget))
	{
		ElectroSphere->SetHomingTarget(true);
	}
}

AAuraElectroSphere* UAuraSpawnElectroSphere::SpawnElectroSphere(const FVector& Location)
{
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Location);
	
	FVector RelativeVector = Location - GetAvatarActorFromActorInfo()->GetActorLocation();
	RelativeVector.Normalize();

	AAuraElectroSphere* Sphere = GetWorld()->SpawnActorDeferred<AAuraElectroSphere>(
		ElectroSphereClass,
		SpawnTransform,
		GetOwningActorFromActorInfo(),
		CurrentActorInfo->PlayerController->GetPawn(),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	Sphere->ProjectileMovement->SetVelocityInLocalSpace(FVector(RelativeVector.X * MovementSpeed, RelativeVector.Y * MovementSpeed, 0));
	
	Sphere->SetDamageDeltaSecond(DamageDeltaSecond);
	Sphere->SetMovementSpeed(MovementSpeed);
	Sphere->SetTraceRadius(TraceRadius);
	Sphere->SetDamageRadius(DamageRadius);
	Sphere->SetFollowToTarget(bFollowTarget);

	Sphere->SetAdditionalTargets(AdditionalTargets);
	Sphere->SetMaxNumShockTargets(MaxNumShockTargets);
	
	Sphere->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Sphere->SetOwner(GetAvatarActorFromActorInfo());

	Sphere->FinishSpawning(SpawnTransform);
	ElectroSphere = Sphere;
	
	return Sphere;
}