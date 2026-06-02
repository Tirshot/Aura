// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/ChainedLightningActor.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Aura/Aura.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interaction/CombatInterface.h"

AChainedLightningActor::AChainedLightningActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	CueStart = CreateDefaultSubobject<USceneComponent>("CueStart");
	SetRootComponent(CueStart);
	
	// 큐 끝남 감지
	StartSphere = CreateDefaultSubobject<USphereComponent>("StartSphere");
	StartSphere->SetupAttachment(CueStart);
	StartSphere->SetCollisionObjectType(ECC_Projectile);
	StartSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	StartSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	StartSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	StartSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap);
	StartSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	CueEnd = CreateDefaultSubobject<USceneComponent>("CueEnd");
	CueEnd->SetupAttachment(CueStart);
	CueEnd->SetRelativeLocation(CueStart->GetComponentLocation() + FVector(150.f, 0, 0));
	
	// 데미지, 충돌 감지
	Sphere->SetupAttachment(CueEnd);
	
	ProjectileMovement->bIsHomingProjectile = false;
}

void AChainedLightningActor::BeginPlay()
{
	Super::BeginPlay();

}

void AChainedLightningActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (CurrentChain >= MaxChain)
	{
		Destroy();
		return;
	}
	
	if (bTailing)
	{
		FVector CurrentLocation = GetActorLocation();
		FVector TargetLocation = CueEnd->GetComponentLocation();
        
		float InterpSpeed = 1000.f; 
		FVector NewLocation = FMath::VInterpConstantTo(CurrentLocation, TargetLocation, DeltaSeconds, InterpSpeed);
        
		SetActorLocation(NewLocation);

		if (FVector::Dist(NewLocation, TargetLocation) < 10.f)
		{
			TArray<AActor*> OverlappedActors;
			TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

			// 현재 투사체와 주인에게는 반사 금지
			ActorsToIgnore.Add(this);
			ActorsToIgnore.Add(GetOwner());
          
			// 반사 대상에게 한번만 튕김
			AActor* CurrentTargetActor = CueEnd->GetAttachParentActor();
			if (CurrentTargetActor)
			{
				ActorsToIgnore.Add(CurrentTargetActor);
			}

			UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
				this,
				OverlappedActors,
				ActorsToIgnore,
				ChainRadius,
				TargetLocation);

			TArray<AActor*> NearestTarget;
			UAuraAbilitySystemLibrary::GetClosestTargets(5, OverlappedActors, NearestTarget, TargetLocation);
          
			// IgnoreActor가 꽉 찼는데, 아직 튕길 대상이 있으면 IgnoreActor 초기화
			if (NearestTarget.Num() == 0 && MaxChain - CurrentChain > 0)
			{
				ActorsToIgnore.Reset();
				ActorsToIgnore.Add(this); 
				ActorsToIgnore.Add(GetOwner());
       
				if (CurrentTargetActor)
				{
					ActorsToIgnore.Add(CurrentTargetActor);
				}

				UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
					this,
					OverlappedActors,
					ActorsToIgnore,
					ChainRadius,
					TargetLocation
				);

				UAuraAbilitySystemLibrary::GetClosestTargets(5, OverlappedActors, NearestTarget, TargetLocation);
			}
			
			// 다음 타겟 추적
			if (NearestTarget.Num() > 0 && NearestTarget[0] != nullptr)
			{
				CurrentChain++;
				
				AActor* NextEnemy = NearestTarget[0];
				FVector NextTargetLocation = NextEnemy->GetActorLocation();

				FRotator Rotation = (NextTargetLocation - TargetLocation).Rotation();

				FTransform SpawnTransform;
				SpawnTransform.SetLocation(TargetLocation);
				SpawnTransform.SetRotation(Rotation.Quaternion());

				AChainedLightningActor* Projectile = GetWorld()->SpawnActorDeferred<AChainedLightningActor>(
				   GetClass(),
				   SpawnTransform,
				   GetOwner(),
				   nullptr,
				   ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

				if (Projectile)
				{
					Projectile->ActorsToIgnore = ActorsToIgnore;
					Projectile->DamageReductionRatio = DamageReductionRatio;
					Projectile->MaxChain = MaxChain;
					Projectile->CurrentChain = CurrentChain;
					Projectile->SetOwner(GetOwner());
					
					// 반사될수록 데미지는 배율로 감소
					Projectile->DamageEffectParams = DamageEffectParams;
					Projectile->DamageEffectParams.BaseDamage = FMath::RoundToInt(DamageEffectParams.BaseDamage * DamageReductionRatio);
					Projectile->FinishSpawning(SpawnTransform);
				}
			}
			
			//
			Destroy();
		}
	}
	else
	{
		// CueEnd의 길이를 조금씩 늘리기
		FVector CurrentRelativeLoc = CueEnd->GetRelativeLocation();
       
		if (CurrentRelativeLoc.X < Length)
		{
			float ExtendSpeed = 1500.f; 
          
			float NewX = CurrentRelativeLoc.X + (ExtendSpeed * DeltaSeconds);
			NewX = FMath::Min(NewX, Length); 
          
			CueEnd->SetRelativeLocation(FVector(NewX, 0.f, 0.f));
		}
	}
}

void AChainedLightningActor::Destroyed()
{
	if (bHit == false)
		OnHit();

	Super::Destroyed();
}

void AChainedLightningActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
		return;
		
	if (bCheckValidOverlap && !IsValidOverlap(OtherActor))
		return;

	if (bHit == false)
		OnHit();

	if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OtherActor))
	{
		// ASC를 가진 액터 데미지 판정
		ApplyDamage(OtherActor, SweepResult);
	}
	
	// 파괴하지 않고, CueStart가 다가오도록 하기
	CueEnd->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	
	if (OtherActor->Implements<UCombatInterface>())
	{
		CueEnd->AttachToComponent(OtherActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	
		if (ProjectileMovement) 
		{
			ProjectileMovement->StopMovementImmediately();
			ProjectileMovement->SetComponentTickEnabled(false);
		}
	}
	
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	bTailing = true;
}
