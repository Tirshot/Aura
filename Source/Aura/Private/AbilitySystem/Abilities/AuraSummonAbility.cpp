// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraSummonAbility.h"

TArray<FVector> UAuraSummonAbility::GetSpawnLocations()
{
	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();

	int Attempt = 0;
	
	// ������ �Ÿ����� ��ȯ�� ��ġ
	const float DeltaSpread = SpawnSpread / NumMinions;

	// Z�� ���� �¿� 45���� ���⺤��
	const FVector LeftSpread = Forward.RotateAngleAxis(-SpawnSpread / 2, FVector::UpVector);
	TArray<FVector> SpawnLocations;

	int idx = 0;
	
	while ( idx < NumMinions )
	{
		// ���� ���� �� ȸ��
		const FVector Direction = LeftSpread.RotateAngleAxis(DeltaSpread * idx, FVector::UpVector);
		FVector DirectionVector = Direction * FMath::FRandRange(MinSpawnDistance, MaxSpawnDistance - 50.f * Attempt);
		FVector ChosenSpawnLocation = Location + DirectionVector;

		FHitResult Hit;
			
		if (GetWorld()->LineTraceSingleByChannel(Hit, ChosenSpawnLocation + FVector(0.f, 0.f, 400.f), ChosenSpawnLocation - FVector(0.f, 0.f, 400.f), ECC_Visibility))
		{
			if (Hit.bBlockingHit)
			{
				ChosenSpawnLocation = Hit.ImpactPoint;
				SpawnLocations.Add(ChosenSpawnLocation);
				idx++;
			}
		}
			
		Attempt ++;

		if (Attempt > 15)
			break;
	}
	return SpawnLocations;
}

TSubclassOf<APawn> UAuraSummonAbility::GetRandomMinionClass()
{
	// ���� ����
	int32 Selection = FMath::RandRange(0, MinionClasses.Num() - 1);

	return MinionClasses[Selection];
}
