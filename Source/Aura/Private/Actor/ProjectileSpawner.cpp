// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/ProjectileSpawner.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AuraAbilityTypes.h"
#include "Actor/AuraProjectile.h"
#include "Character/AuraEnemy.h"

AProjectileSpawner::AProjectileSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComp"));
	SetRootComponent(SceneComp);
}

void AProjectileSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &AProjectileSpawner::SpawnProjectile, SpawnInterval, true);
	}
}

void AProjectileSpawner::SpawnProjectile()
{
	if (!ProjectileClass)
		return;
	
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(GetActorLocation());
	SpawnTransform.SetRotation(GetActorRotation().Quaternion());

	AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(
		ProjectileClass,
		SpawnTransform,
		DummyEnemy,
		GetInstigator(),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	Projectile->bCheckValidOverlap = false;
	Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Projectile->FinishSpawning(SpawnTransform);
}

FDamageEffectParams AProjectileSpawner::MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor) const
{
	FDamageEffectParams Params;
	
	Params.WorldContextObject = DummyEnemy;
	Params.SourceAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(DummyEnemy);
	Params.TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	// ASC
	Params.DamageGameplayEffectClass = DamageEffectClass;
	
	// 데미지
	Params.BaseDamage = Damage;
	Params.DamageType = DamageType;

	// 디버프
	Params.DebuffChance = DebuffChance;
	Params.DebuffDamage = DebuffDamage;
	Params.DebuffDuration = DebuffDuration;
	Params.DebuffFrequency = DebuffFrequency;

	// 넉백
	Params.KnockbackForceMagnitude = KnockbackForceMagnitude;
	Params.KnockbackChance = KnockbackChance;

	// 넉백 계산
	const bool bKnockback = FMath::RandRange(1, 100) < Params.KnockbackChance;
	
	if (IsValid(TargetActor))
	{
		FRotator Rotation = (TargetActor->GetActorLocation() - GetActorLocation()).Rotation();
		
		FVector ToTarget = Rotation.Vector();
		Params.KnockbackForce = ToTarget * KnockbackForceMagnitude;
	}
	
	// 타겟 방향으로 충격파와 넉백 실시
	if (bKnockback == false)
	{
		Params.KnockbackForce = FVector::ZeroVector;
	}

	return Params;
}