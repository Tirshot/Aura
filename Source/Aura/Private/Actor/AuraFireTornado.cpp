// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraFireTornado.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Aura/Aura.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AAuraFireTornado::AAuraFireTornado()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
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
	SetReplicateMovement(true);
	SetLifeSpan(LifeSpan);
	
	if (HasAuthority())
	{
		// 데미지 반경에 따라 메시 크기 조절
		OnRep_DamageRadius();
	}
	
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

void AAuraFireTornado::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AAuraFireTornado, DamageRadius);
}

void AAuraFireTornado::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	FRotator DeltaRotation = FRotator(0.f, SpinDegreePerSecond * DeltaSeconds, 0.f);
	AddActorWorldRotation(DeltaRotation);

	if (!GetOwner() || GetOwner()->IsPendingKillPending())
	{
		Destroy();
		return;
	}

	if (!OverlappingActors.IsEmpty())
	{
		AActor* TargetActor = nullptr;
	
		// 가장 먼 대상 스캔
		int32 OverlappingActorsNum = OverlappingActors.Num() - 1;
		for (int i = OverlappingActorsNum; i >= 0; i--)
		{
			AActor* Target = OverlappingActors[i];
			if (!IsValid(Target))
				continue;

			if (!IsValidOverlap(Target))
				continue;
			
			// 아군 추적 금지
			if (!UAuraAbilitySystemLibrary::IsNotFriend(GetOwner(), Target))
				continue;
			
			TargetActor = Target;
			break;
		}
		
		// 타겟 없으면 제자리 회전
		if (TargetActor)
		{
			FVector TargetLocation = TargetActor->GetActorLocation();
			FVector TornadoLocation = GetActorLocation();

			TargetLocation.Z = TornadoLocation.Z;

			FVector NewLocation = FMath::VInterpTo(TornadoLocation, TargetLocation, DeltaSeconds, MovementSpeed);
			SetActorLocation(NewLocation, true);
		}
	}
}

void AAuraFireTornado::Destroyed()
{
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
		if (!IsValidOverlap(OtherActor))
			continue;

		if (HasAuthority())
		{
			if (auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
			{
				bool bIsKnockBack = true;
				
				// 벡터를 구하고 정규화
				FVector ToCenter = GetActorLocation() - OtherActor->GetActorLocation();
				ToCenter.Normalize();
				
				// 보스는 넉백 무효
				if (OtherActor->ActorHasTag(FName("Boss")))
				{
					bIsKnockBack = false;
					DamageEffectParams.KnockbackChance = 0.f;
				}

				if (bIsKnockBack)
				{
					int32 RandValue = FMath::RandRange(0, 100);
					int32 KnockbackChance = DamageEffectParams.KnockbackChance;

					if (RandValue <= KnockbackChance)
					{
						const FVector KnockbackDirection = ToCenter;
						const FVector KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;
						DamageEffectParams.KnockbackForce = KnockbackForce;
					}
					else
					{
						DamageEffectParams.KnockbackForce = FVector::ZeroVector;
					}
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
	if (!SourceAvatarActor)
		return false;
	
	if (SourceAvatarActor == OtherActor)
		return false;

	// 아군 일 경우
	if (!UAuraAbilitySystemLibrary::IsNotFriend(SourceAvatarActor, OtherActor))
		return false;

	return true;
}

void AAuraFireTornado::OnRep_DamageRadius()
{
	// DamageRadius에 의해 메시 크기 스케일
	float NewScaleValue = DamageRadius / 200.f;
	FVector NewScale(NewScaleValue);
	
	Mesh->SetWorldScale3D(NewScale);
}
