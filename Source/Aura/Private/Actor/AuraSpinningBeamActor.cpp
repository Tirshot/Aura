// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraSpinningBeamActor.h"

#include "AbilitySystemGlobals.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Abilities/AuraSpinningBeam.h"
#include "Aura/Aura.h"
#include "Components/AudioComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

AAuraSpinningBeamActor::AAuraSpinningBeamActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	SceneComponent = CreateDefaultSubobject<USceneComponent>("SceneComponent");
	SceneComponent->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	SceneComponent->SetupAttachment(RootComponent);

	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bInterpRotation = false;
	
	ProjectileMovement->InitialSpeed = 100000.f;
	ProjectileMovement->MaxSpeed = 100000.f;
	ProjectileMovement->HomingAccelerationMagnitude = 100000.f;
	
	ProjectileMovement->Deactivate();
	
	// 사운드 컴포넌트
	LoopingSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("LoopingSoundComponent"));
	LoopingSoundComponent->SetupAttachment(RootComponent);
	LoopingSoundComponent->bAutoActivate = true;
}

void AAuraSpinningBeamActor::BeginPlay()
{
	Super::BeginPlay();
	
	SetReplicateMovement(false);

	if (Owner)
	{
		OrbitCenter = Owner->GetActorLocation();
	}

	// 회전각
	SpinningAngle = InitialAngle;
	
	if (LoopingSound && LoopingSoundComponent)
	{
		LoopingSoundComponent->SetSound(LoopingSound);
		LoopingSoundComponent->Play();
	}
}

void AAuraSpinningBeamActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	if (Owner)
	{
		OrbitCenter = Owner->GetActorLocation();
	}
	
	SpinningAngle += AngleSpeed * DeltaSeconds;
	
	FVector InitialVector = FVector(OrbitRadius, 0.f, 0.f).RotateAngleAxis(FMath::RadiansToDegrees(InitialAngle), FVector::UpVector);
	FVector RotatedVector = InitialVector.RotateAngleAxis(SpinningAngle, FVector::UpVector);
	
	SetActorLocation(GetOwner()->GetActorLocation() + RotatedVector + FVector(0,0,50.f));
	SetActorRotation(FRotator(-90.f, InitialAngle, 0.f));

	FVector Tangent = FVector(-FMath::Sin(SpinningAngle), FMath::Cos(SpinningAngle), 0.f);
	
	// Z축 기준으로 회전만 적용 (Yaw만)
	FRotator LookRotation = Tangent.Rotation();

	SceneComponent->SetRelativeRotation(FRotator(0.f, InitialAngle + LookRotation.Yaw, 0.f));
	
	// 틱 데미지 계산
	TimeElapsed += DeltaSeconds;
	DestroyTimeElapsed += DeltaSeconds;
	if (TimeElapsed >= DamageDeltaTime)
	{
		// 데미지 입히기
		ApplyTickDamage();
		TimeElapsed = 0.f;
	}
	
	if (DestroyTimeElapsed >= LifeSpan)
	{
		Destroy();
	}
}

void AAuraSpinningBeamActor::Destroyed()
{
	if (OwnedAbility)
		Cast<UAuraSpinningBeam>(OwnedAbility)->BeamDestroyed();
	
	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->Deactivate();
	}
	
	Super::Destroyed();
}

void AAuraSpinningBeamActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AAuraSpinningBeamActor::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	
}

void AAuraSpinningBeamActor::ApplyTickDamage()
{
	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!SourceASC)
		return;
	
	// 캐릭터 중심에서 이 액터까지의 라인 트레이스 멀티
	TArray<FHitResult> HitResults;
	
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());
	
	FVector Start = GetOwner()->GetActorLocation();
	FVector End = GetActorLocation();
	
	bool bIsHit = GetWorld()->LineTraceMultiByChannel(
		HitResults,
		Start,
		End,
		ECC_GASActor,
		QueryParams);
	
	if (!bIsHit)
		return;
	
	for (const FHitResult& HitResult : HitResults)
	{
		AActor* TargetActor = HitResult.GetActor();
		if (!IsValid(TargetActor) || !UAuraAbilitySystemLibrary::IsNotFriend(GetOwner(), TargetActor))
			continue;
		
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), 
			BeamImpactNiagara,
			HitResult.ImpactPoint,
			FRotator::ZeroRotator
		);
		
		UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(HitResult.GetActor());
		if (!TargetASC)
			continue;
		
		DamageEffectParams.TargetAbilitySystemComponent = TargetASC;
		DamageEffectParams.SourceAbilitySystemComponent = SourceASC;
		DamageEffectParams.WorldContextObject = this;
		
		// 넉백 - 시전자에서 이 액터 방향
		FVector BeamDirection = (GetActorLocation() - Start).GetSafeNormal();
		const FVector DeathImpulse = BeamDirection * DamageEffectParams.DeathImpulseMagnitude;
		DamageEffectParams.DeathImpulse = DeathImpulse;

		// 접선 벡터 계산 (위쪽 벡터와 외적) - 이전 벡터에서 다음 벡터로 감았을 때 오른손의 검지 방향
		FVector TangentVector = FVector::CrossProduct(FVector::UpVector, BeamDirection).GetSafeNormal();
		DamageEffectParams.KnockbackForce = FVector::ZeroVector;

		const bool bKnockback = FMath::RandRange(1, 100) < DamageEffectParams.KnockbackChance;
		if (bKnockback)
		{
			const FVector KnockbackDirection = TangentVector;
			const FVector KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;
			DamageEffectParams.KnockbackForce = KnockbackForce;
		}
		
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, TangentVector, TargetActor->GetActorRotation(), 1.f);
		UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
	}
}
