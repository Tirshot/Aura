// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraSummonAbility.h"

TArray<FVector> UAuraSummonAbility::GetSpawnLocations()
{
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();
	
	// 일정한 거리마다 소환수 배치
	const float DeltaSpread = SpawnSpread / NumMinions;

	// Z축 기준 좌우 45도의 방향벡터
	const FVector LeftSpread = Forward.RotateAngleAxis(-SpawnSpread / 2, FVector::UpVector);
	TArray<FVector> SpawnLocations;
	for (int32 i = 0; i < NumMinions; i++)
	{
		// 나눈 각도 씩 회전
		const FVector Direction = LeftSpread.RotateAngleAxis(DeltaSpread * i, FVector::UpVector);
		FVector ChosenSpawnLocation = Location + Direction * FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance);
		
		FHitResult Hit;
		GetWorld()->LineTraceSingleByChannel(Hit, ChosenSpawnLocation + FVector(0.f, 0.f, 400.f), ChosenSpawnLocation - FVector(0.f, 0.f, 400.f), ECC_Visibility);

		if (Hit.bBlockingHit)
		{
			ChosenSpawnLocation = Hit.ImpactPoint;
		}
		SpawnLocations.Add(ChosenSpawnLocation);
	}

	return SpawnLocations;
}

TSubclassOf<APawn> UAuraSummonAbility::GetRandomMinionClass()
{
	// 랜덤 선택
	int32 Selection = FMath::RandRange(0, MinionClasses.Num() - 1);

	return MinionClasses[Selection];
}
