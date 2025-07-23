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

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
public:
	virtual void CheckAbilityUpgrades(FGameplayTag AbilityTag) override;

	UFUNCTION(BlueprintCallable)
	void ReceivedMouseHitResult(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION(BlueprintCallable)
	void ReadyToSpawnShards();

	UFUNCTION(BlueprintCallable)
	void SpawnShards();

	UFUNCTION(BlueprintCallable)
	void EndSpawnShards();

	UFUNCTION(BlueprintCallable)
	void ApplyRadialDamage(TArray<AActor*>& OutOverlappingActors, float OuterRadius);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<APointCollection> PointCollectionClass = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TArray<USceneComponent*> GroundPoints;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<APointCollection> PointCollection = nullptr;
	
protected:
	UPROPERTY(BlueprintReadWrite)
	int32 NumPoints = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 AdditionalShards = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 MaxNumShards = 10;

	UPROPERTY(VisibleAnywhere)
	bool bIsFirstShardLarge = false;

	UPROPERTY(EditDefaultsOnly)
	float UpgradeFirstShardSizeMultiplier = 150.f;

	UPROPERTY(BlueprintReadWrite)
	FVector CurrentMouseLocation = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadWrite)
	FVector ShardSpawnLocation = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadWrite)
	FRotator ShardSpawnRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite)
	FTimerHandle ShardSpawnTimer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float SpawnShardsDeltaTime = 0.2f;

	int32 Idx = 0;
};
