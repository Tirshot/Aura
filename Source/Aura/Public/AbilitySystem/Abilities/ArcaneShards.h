// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "ArcaneShards.generated.h"

class APointCollection;

UCLASS()
class AURA_API UArcaneShards : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()
	
public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
public:
	virtual void CheckAbilityUpgrades() override;
	
	UFUNCTION(BlueprintCallable)
	void CreatePointCollection();
	
	UFUNCTION(BlueprintCallable)
	void ReadyToSpawnShards();

	UFUNCTION(BlueprintCallable)
	void SpawnShards();

	UFUNCTION(BlueprintCallable)
	void SpawnCueAndApplyDamage();

	UFUNCTION(BlueprintCallable)
	void EndSpawnShards();

	UFUNCTION(BlueprintCallable)
	void ApplyRadialDamage(TArray<AActor*>& OutOverlappingActors, float OuterRadius);
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<APointCollection> PointCollectionClass = nullptr;

protected:
	UPROPERTY(BlueprintReadOnly)
	TArray<USceneComponent*> GroundPoints;

	UPROPERTY()
	TObjectPtr<APointCollection> PointCollection = nullptr;
	
	UPROPERTY()
	int32 NumPoints = 0;

	UPROPERTY()
	int32 AdditionalShards = 0;

	UPROPERTY()
	int32 MaxNumShards = 10;

	UPROPERTY()
	bool bIsFirstShardLarge = false;

	UPROPERTY()
	float UpgradeFirstShardSizeMultiplier = 150.f;
	
	UPROPERTY(BlueprintReadOnly)
	FVector ShardSpawnLocation = FVector::ZeroVector;
	
	UPROPERTY()
	FRotator ShardSpawnRotation = FRotator::ZeroRotator;

	UPROPERTY()
	FTimerHandle ShardSpawnTimer;

	UPROPERTY(EditDefaultsOnly)
	float SpawnShardsDeltaTime = 0.2f;
	
	UPROPERTY(EditDefaultsOnly)
	bool bSpawnShardDelayed = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ShardSpawnDelay = 0.5f;

	int32 Idx = 0;
};
