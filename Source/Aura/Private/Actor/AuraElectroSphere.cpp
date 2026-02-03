// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraElectroSphere.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Character/AuraEnemy.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interaction/CombatInterface.h"

AAuraElectroSphere::AAuraElectroSphere()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(GetRootComponent());

	ProjectileMovement->SetVelocityInLocalSpace(FVector(MovementSpeed, 0.f, 0.f));
}

void AAuraElectroSphere::BeginPlay()
{
	Super::BeginPlay();

	// DamageRadius에 의해 메시 크기 스케일
	float NewScaleValue = DamageRadius / 200.f;
	FVector NewScale(NewScaleValue);
	
	Mesh->SetWorldScale3D(NewScale);

	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUFunction(this, FName("ApplyMainDamage"));

	GetWorldTimerManager().SetTimer(
		TimerHandle,
		TimerDelegate,
		DamageDeltaSecond,
		true
		);
}

void AAuraElectroSphere::OnHit()
{
	bHit = true;
}

void AAuraElectroSphere::Destroyed()
{
	// if (LoopingSoundComponent)
	// {
	// 	LoopingSoundComponent->Stop();
	// 	LoopingSoundComponent->DestroyComponent();
	// }
	
	Super::Destroyed();
}

void AAuraElectroSphere::ApplyMainDamage()
{
	MainOverlappingActors.Empty();
	
	TArray<AActor*> IgnoreActors;
	IgnoreActors.AddUnique(GetOwner());
	
	UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
		this,
		MainOverlappingActors,
		IgnoreActors,
		DamageRadius,
		GetActorLocation());

	for (AActor* OtherActor : MainOverlappingActors)
	{
		if (IsValidOverlap(OtherActor) == false)
			continue;

		if (HasAuthority())
		{
			if (auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
			{
				bool bIsKnockBack = false;
				
				// 벡터를 구하고 정규화
				FVector ToCenter = GetActorLocation() - OtherActor->GetActorLocation();
				ToCenter.Z = GetActorLocation().Z - Sphere->GetScaledSphereRadius();
				ToCenter.Normalize();
				
				// 보스는 넉백 무효
				if (OtherActor->ActorHasTag(FName("Boss")))
				{
					bIsKnockBack = false;
					DamageEffectParams.KnockbackChance = 0.f;
				}

				if (bIsKnockBack)
				{
					const FVector KnockbackDirection = ToCenter;
					const FVector KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;
					DamageEffectParams.KnockbackForce = KnockbackForce;
				}

				FVector DistanceVector = (OtherActor->GetActorLocation()-GetActorLocation());
				float Distance = DistanceVector.Length();
				
				if (Distance <= DamageRadius)
				{
					const FVector DeathImpulse = ToCenter * DamageEffectParams.DeathImpulseMagnitude;
					DamageEffectParams.DeathImpulse = DeathImpulse;
					DamageEffectParams.TargetAbilitySystemComponent = TargetASC;
				
					UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
				}
			}
		}
	}
}

void AAuraElectroSphere::DetectAdditionalTargets(TArray<AActor*> ActorsToIgnore)
{
	TArray<AActor*> CurrentOverlappingActors;
	UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
		this,
		CurrentOverlappingActors,
		ActorsToIgnore,
		TraceRadius,
		GetActorLocation());
	
	// 액터로부터 가장 가까운 어빌리티의 추가 타겟 갯수 만큼만 공격 
	TArray<AActor*> CurrentClosestTargets;
	UAuraAbilitySystemLibrary::GetClosestTargets(
		NumAdditionalTargets,
		CurrentOverlappingActors,
		CurrentClosestTargets,
		GetActorLocation());
	
	CurrentClosestTargets.RemoveAll([](AActor* OverlappingActor)
	{
		return OverlappingActor->ActorHasTag("Player");
	});
	
	// 범위를 벗어난 적에게서 게임플레이 큐 제거
	FGameplayTag GameplayCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.ElectroSphere");
	
	TArray<AActor*> EnemyToRemoveCue;
	for (AActor* Enemy : AdditionalOverlappingActors)
	{
		// 범위 밖이거나 사망
		if (!CurrentClosestTargets.Contains(Enemy) || !IsValid(Enemy))
		{
			if (UAbilitySystemComponent* EnemyASC = Cast<AAuraEnemy>(Enemy)->GetAbilitySystemComponent())
			{
				EnemyASC->RemoveGameplayCue(GameplayCueTag);

				if (Enemy->Implements<UCombatInterface>())
				{
					ICombatInterface::Execute_SetIsBeingShocked(Enemy, false);
				}
			}
		}
		EnemyToRemoveCue.Add(Enemy);
	}
	
	for (AActor* Enemy : EnemyToRemoveCue)
	{
		AdditionalOverlappingActors.Remove(Enemy);
	}

	FGameplayCueParameters CueParams;
	CueParams.TargetAttachComponent = GetRootComponent();

	// 구체 주위의 적에게 게임플레이 큐 추가
	for (AActor* CurrentTarget : CurrentClosestTargets)
	{
		if (!AdditionalOverlappingActors.Contains(CurrentTarget))
		{
			if (UAbilitySystemComponent* TargetASC = Cast<AAuraEnemy>(CurrentTarget)->GetAbilitySystemComponent())
			{
				CueParams.SourceObject = CurrentTarget;

				// 큐 중복 추가 방지
				if (!TargetASC->HasMatchingGameplayTag(GameplayCueTag))
					TargetASC->AddGameplayCue(GameplayCueTag, CueParams);

				if (CurrentTarget->Implements<UCombatInterface>())
				{
					ICombatInterface::Execute_SetIsBeingShocked(CurrentTarget, true);
				}
			}
		}
		AdditionalOverlappingActors.Add(CurrentTarget);
	}

	//
	HomingNearestTarget();
}

void AAuraElectroSphere::HomingNearestTarget()
{
	if (!bHomingTarget)
		return;

	TArray<AActor*> NearestTarget;

	UAuraAbilitySystemLibrary::GetClosestTargets(
		1,
		AdditionalOverlappingActors,
		NearestTarget,
		GetActorLocation()
		);

	if (NearestTarget.IsEmpty())
		return;
	
	ProjectileMovement->HomingTargetComponent = NearestTarget[0]->GetRootComponent();
	ProjectileMovement->bIsHomingProjectile = true;
	ProjectileMovement->HomingAccelerationMagnitude = 600.f;
}

float AAuraElectroSphere::GetSphereRadius()
{
	if (!IsValid(Sphere))
		return 0.f;

	return Sphere->GetScaledSphereRadius();
}

void AAuraElectroSphere::SetMovementSpeed(float InSpeed)
{
	MovementSpeed = InSpeed;

	// 단위 벡터
	FVector CurrentDirection = ProjectileMovement->Velocity.GetSafeNormal();
	FVector NewVelocity = CurrentDirection * MovementSpeed;
	
	ProjectileMovement->SetVelocityInLocalSpace(NewVelocity);
	ProjectileMovement->MaxSpeed = InSpeed;
	ProjectileMovement->InitialSpeed = InSpeed;
}

void AAuraElectroSphere::AddMovementSpeed(float InSpeed)
{
	MovementSpeed += InSpeed;

	FVector CurrentVelocity = ProjectileMovement->Velocity;
	float CurrentSpeed = CurrentVelocity.Size();

	float NewSpeed = FMath::Max(50.f, CurrentSpeed + InSpeed);

	FVector NewVelocity = CurrentVelocity.GetSafeNormal() * NewSpeed;

	ProjectileMovement->SetVelocityInLocalSpace(NewVelocity);
}
