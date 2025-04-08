// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/PointCollection.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"

APointCollection::APointCollection()
{
	PrimaryActorTick.bCanEverTick = false;

	Pt_0 = CreateDefaultSubobject<USceneComponent>("Pt_0");
	ImmutablePts.Add(Pt_0);
	SetRootComponent(Pt_0);

	Pt_1 = CreateDefaultSubobject<USceneComponent>("Pt_1");
	ImmutablePts.Add(Pt_1);
	Pt_1->SetupAttachment(GetRootComponent());

	Pt_2 = CreateDefaultSubobject<USceneComponent>("Pt_2");
	ImmutablePts.Add(Pt_2);
	Pt_2->SetupAttachment(GetRootComponent());

	Pt_3 = CreateDefaultSubobject<USceneComponent>("Pt_3");
	ImmutablePts.Add(Pt_3);
	Pt_3->SetupAttachment(GetRootComponent());

	Pt_4 = CreateDefaultSubobject<USceneComponent>("Pt_4");
	ImmutablePts.Add(Pt_4);
	Pt_4->SetupAttachment(GetRootComponent());

	Pt_5 = CreateDefaultSubobject<USceneComponent>("Pt_5");
	ImmutablePts.Add(Pt_5);
	Pt_5->SetupAttachment(GetRootComponent());

	Pt_6 = CreateDefaultSubobject<USceneComponent>("Pt_6");
	ImmutablePts.Add(Pt_6);
	Pt_6->SetupAttachment(GetRootComponent());

	Pt_7 = CreateDefaultSubobject<USceneComponent>("Pt_7");
	ImmutablePts.Add(Pt_7);
	Pt_7->SetupAttachment(GetRootComponent());

	Pt_8 = CreateDefaultSubobject<USceneComponent>("Pt_8");
	ImmutablePts.Add(Pt_8);
	Pt_8->SetupAttachment(GetRootComponent());

	Pt_9 = CreateDefaultSubobject<USceneComponent>("Pt_9");
	ImmutablePts.Add(Pt_9);
	Pt_9->SetupAttachment(GetRootComponent());

	Pt_10 = CreateDefaultSubobject<USceneComponent>("Pt_10");
	ImmutablePts.Add(Pt_10);
	Pt_10->SetupAttachment(GetRootComponent());
}

TArray<USceneComponent*> APointCollection::GetGroundPoints(int32 NumPoints, float YawOverride)
{
	checkf(ImmutablePts.Num() >= NumPoints, TEXT("ATTEMPTED TO ACCESS IMMUTABLEPTS OUT OF BOUNDS."));

	TArray<USceneComponent*> ArrayCopy;

	for (USceneComponent* Pt : ImmutablePts)
	{
		// 충분한 점 갯수를 얻으면 리턴
		if (ArrayCopy.Num() >= NumPoints)
			return ArrayCopy;

		FVector PointLocation = Pt->GetComponentLocation();

		// 중앙 기준으로 중앙 점을 제외한 각 점을 정렬
		if (Pt != Pt_0)
		{
			FVector ToPoint = PointLocation - GetActorLocation();
			ToPoint = ToPoint.RotateAngleAxis(YawOverride, FVector::UpVector);
			Pt->SetWorldLocation(GetActorLocation() + ToPoint);
		}


		// 라인 트레이스 - Z축 기준 1000.f
		const FVector UpLocation = FVector(PointLocation.X, PointLocation.Y, PointLocation.Z + 300.f);
		const FVector DownLocation = FVector(PointLocation.X, PointLocation.Y, PointLocation.Z - 300.f);

		FHitResult HitResult;
		TArray<AActor*> IgnoreActors;

		// 액터를 무시하고(액터에 의한 고저차 무시) 점에 대해서만 라인트레이스 실시
		UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(this, IgnoreActors, TArray<AActor*>(), 1500.f, PointLocation);

		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActors(IgnoreActors);
		GetWorld()->LineTraceSingleByProfile(HitResult, UpLocation, DownLocation, FName("BlockAll"), QueryParams);

		// 땅에 대해 투사, 지면에 수직한 방향으로 회전
		const FVector AdjustedLocation = FVector(PointLocation.X, PointLocation.Y, HitResult.ImpactPoint.Z);
		Pt->SetWorldLocation(AdjustedLocation);
		Pt->SetWorldRotation(UKismetMathLibrary::MakeRotFromZ(HitResult.ImpactNormal));

		ArrayCopy.Add(Pt);
	}
	return ArrayCopy;
}

void APointCollection::BeginPlay()
{
	Super::BeginPlay();
	
}
