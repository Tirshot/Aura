// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraSummonAbility.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraSummonAbility : public UAuraGameplayAbility
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	TArray<FVector> GetSpawnLocations();

	UFUNCTION(BlueprintPure, category = "Summoning")
	TSubclassOf<APawn> GetRandomMinionClass();

	UPROPERTY(EditDefaultsOnly, category="Summoning")
	int32 NumMinions = 5;

	UPROPERTY(EditDefaultsOnly, category = "Summoning")
	TArray<TSubclassOf<APawn>> MinionClasses;

	UPROPERTY(EditDefaultsOnly, category = "Summoning")
	float MinSpawnDistance = 50.f;

	UPROPERTY(EditDefaultsOnly, category = "Summoning")
	float MaxSpawnDistance = 250.f;

	// 소환 스폰 각도
	UPROPERTY(EditDefaultsOnly, category = "Summoning")
	float SpawnSpread = 90.f;
};
