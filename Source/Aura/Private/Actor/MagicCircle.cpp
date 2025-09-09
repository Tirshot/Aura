// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/MagicCircle.h"
#include "Components/DecalComponent.h"
#include "Interaction/EnemyInterface.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

AMagicCircle::AMagicCircle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");

	MagicCircleDecal = CreateDefaultSubobject<UDecalComponent>("MagicCircleDecal");
	MagicCircleDecal->SetupAttachment(RootComponent);
}

void AMagicCircle::BeginPlay()
{
	Super::BeginPlay();
}

void AMagicCircle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	MagicCircleDecal->DecalSize = FVector(127.f, Radius, Radius);

	// 일정 범위 내에 갇히게 하기
	KeepMagicCircleInRange();
}

void AMagicCircle::KeepMagicCircleInRange()
{
	if (CircleRange > 0.f)
	{
		FVector OwnerLocation = GetOwner()->GetActorLocation();
		FVector CurrentCircleLocation = GetActorLocation();

		float Distance = FVector::Dist(OwnerLocation, CurrentCircleLocation);

		if (Distance >= CircleRange)
		{
			FVector Direction = (CurrentCircleLocation - OwnerLocation).GetSafeNormal();
			FVector NewTargetLocation = OwnerLocation + (Direction * CircleRange);

			SetActorLocation(NewTargetLocation);

			FHitResult HitResult;
			FVector TraceStart = NewTargetLocation + FVector(0, 0, 500.f);
			FVector TraceEnd = NewTargetLocation + FVector(0, 0, -500.f);

			GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility); 

			if (HitResult.bBlockingHit)
			{
				SetActorLocation(HitResult.Location);
			}
		}
	}
}
