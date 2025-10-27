// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraArcaneMissile.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Abilities/AuraArcaneOrbit.h"
#include "Character/AuraCharacterBase.h"
#include "GameFramework/ProjectileMovementComponent.h"

AAuraArcaneMissile::AAuraArcaneMissile()
{
	PrimaryActorTick.bCanEverTick = true;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("StaticMesh");
	Mesh->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Mesh->SetupAttachment(RootComponent);

	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bInterpRotation = false;
	
	ProjectileMovement->InitialSpeed = 100000.f;
	ProjectileMovement->MaxSpeed = 100000.f;
	ProjectileMovement->HomingAccelerationMagnitude = 100000.f;
	
	ProjectileMovement->Deactivate();
}

void AAuraArcaneMissile::BeginPlay()
{
	Super::BeginPlay();

	if (Owner)
	{
		OrbitCenter = Owner->GetActorLocation();
	}

	// 회전각
	SpinningAngle = InitialAngle;
}

void AAuraArcaneMissile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	HomingNearestTarget(DeltaSeconds);
	
	// 타겟 설정되면 궤도 회전 정지
	if (TargetSet)
	{
		// 타겟 생존여부 확인
		if (AActor* TargetActor = ProjectileMovement->HomingTargetComponent->GetOwner())
		{
			if (AAuraCharacterBase* TargetCharacter = Cast<AAuraCharacterBase>(TargetActor))
			{
				if (TargetCharacter->Implements<UCombatInterface>())
				{
					if (ICombatInterface::Execute_IsDead(TargetCharacter))
					{
						SetHasTarget(false);
					}
				}
			}
		}
		return;
	}

	// 각 속도 w = v/r
	float AngleSpeed = OrbitSpeed / OrbitRadius;
	SpinningAngle += AngleSpeed * DeltaSeconds;

	if (Owner)
	{
		OrbitCenter = Owner->GetActorLocation();
	}

	// 상대 좌표 + 회전에서 X축 방향
	float NewX = OrbitCenter.X + OrbitRadius * FMath::Cos(SpinningAngle);
	float NewY = OrbitCenter.Y + OrbitRadius * FMath::Sin(SpinningAngle);
	float Z = OrbitCenter.Z;

	FVector NewLocation = FVector(NewX, NewY, Z);
	SetActorLocation(NewLocation);
	SetActorRotation(FRotator(-90.f, InitialAngle, 0.f));

	FVector Tangent = FVector(-FMath::Sin(SpinningAngle), FMath::Cos(SpinningAngle), 0.f);
	
	// Z축 기준으로 회전만 적용 (Yaw만)
	FRotator LookRotation = Tangent.Rotation();

	Mesh->SetRelativeRotation(FRotator(0.f, InitialAngle + LookRotation.Yaw, 0.f));
}

void AAuraArcaneMissile::Destroyed()
{
	if (UAuraArcaneOrbit* Ability = Cast<UAuraArcaneOrbit>(OwnedAbility))
	{
		Ability->MissileDestroyed(this);
	}
	
	Super::Destroyed();
}

void AAuraArcaneMissile::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);
}

void AAuraArcaneMissile::HomingNearestTarget(float DeltaTime)
{
	TimeElapsed += DeltaTime;

	if (TimeElapsed < InitialDelayDuration)
		return;

	TArray<AActor*> OutOverlappingActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(Owner);
	ActorsToIgnore.Add(this);

	UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
		this,
		OutOverlappingActors,
		ActorsToIgnore, FollowRadius, GetActorLocation());

	for (AActor* Actor : OutOverlappingActors)
	{
		if (ProjectileMovement->HomingTargetComponent == nullptr)
		{
			ProjectileMovement->bIsHomingProjectile = true;
			ProjectileMovement->HomingTargetComponent = Actor->GetRootComponent();

			ProjectileMovement->Activate();
			
			SetHasTarget(true);
		}
	}

	if (ProjectileMovement->HomingTargetComponent == nullptr)
	{
		ProjectileMovement->Deactivate();
			
		SetHasTarget(false);
	}
}
