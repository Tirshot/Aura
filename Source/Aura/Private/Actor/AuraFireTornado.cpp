// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraFireTornado.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Aura/Aura.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"

AAuraFireTornado::AAuraFireTornado()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(Mesh);

	Capsule = CreateDefaultSubobject<UCapsuleComponent>("Capsule");
	
	Capsule->SetCollisionObjectType(ECC_Projectile);
	Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	Capsule->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	Capsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Capsule->SetupAttachment(GetRootComponent());
}

void AAuraFireTornado::BeginPlay()
{
	Super::BeginPlay();
	SetLifeSpan(LifeSpan);
	SetReplicateMovement(true);
	
	LoopingSoundComponent = UGameplayStatics::SpawnSoundAttached(LoopingSound, GetRootComponent());

	// 타이머 설정
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUFunction(this, FName("ApplyDamageAndKnockback"));

	GetWorldTimerManager().SetTimer(
		TimerHandle,
		TimerDelegate,
		DamageDeltaSecond,
		true
		);
}

void AAuraFireTornado::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FRotator DeltaRotation = FRotator(0.f, SpinDegreePerSecond * DeltaSeconds, 0.f);
	AddActorWorldRotation(DeltaRotation);

	if (!OverlappingActors.IsEmpty())
	{
		AActor* TargetActor = OverlappingActors.Last();

		if (!IsValid(TargetActor))
			return;

		FVector TornadoLocation = GetActorLocation();
		FVector ActorLocation = TargetActor->GetActorLocation();

		ActorLocation.Z = TornadoLocation.Z;

		FVector NewLocation = FMath::VInterpTo(TornadoLocation, ActorLocation, DeltaSeconds, MovementSpeed);
		SetActorLocation(NewLocation, true);
	}
}

void AAuraFireTornado::Destroyed()
{
	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}

	// 타이머 리셋
	GetWorldTimerManager().ClearTimer(TimerHandle);
	
	Super::Destroyed();
}

void AAuraFireTornado::ApplyDamageAndKnockback()
{
	OverlappingActors.Empty();
	
	TArray<AActor*> IgnoreActors;
	IgnoreActors.AddUnique(GetOwner());
	
	UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
		this,
		OverlappingActors,
		IgnoreActors,
		FollowRadius,
		GetActorLocation());

	// 1. 중앙으로 끌어 당김
	// 2. 틱 데미지 부여
	// 3. 범위 밖의 액터를 향해 느리게 움직임
	
	for (AActor* OtherActor : OverlappingActors)
	{
		if (IsValidOverlap(OtherActor) == false)
			continue;

		if (HasAuthority())
		{
			if (auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
			{
				bool bIsKnockBack = true;
				
				// 벡터를 구하고 정규화
				FVector ToCenter = GetActorLocation() - OtherActor->GetActorLocation();
				ToCenter.Z = GetActorLocation().Z - Capsule->GetScaledCapsuleHalfHeight();
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

bool AAuraFireTornado::IsValidOverlap(AActor* OtherActor)
{
	if (DamageEffectParams.SourceAbilitySystemComponent == nullptr)
		return false;

	AActor* SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor();

	if (SourceAvatarActor == OtherActor)
		return false;

	if (!UAuraAbilitySystemLibrary::IsNotFriend(SourceAvatarActor, OtherActor))
		return false;

	return true;
}