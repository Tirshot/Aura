// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AuraArcaneOrbit.generated.h"

class AAuraArcaneMissile;
/**
 * 
 */
UCLASS()
class AURA_API UAuraArcaneOrbit : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

protected:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;
	
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual void CheckAbilityUpgrades() override;
public:
	UFUNCTION(BlueprintCallable)
	TArray<AAuraArcaneMissile*> SpawnArcaneMissiles();

	UFUNCTION()
	void DestroyAllMissiles();

	UFUNCTION()
	void MissileDestroyed(AActor* DestroyedMissile);

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraArcaneMissile> ArcaneMissileClass;

	UPROPERTY()
	TArray<AAuraArcaneMissile*> Missiles;
	
	UPROPERTY(EditDefaultsOnly)
	int32 NumMissiles = 3;

	UPROPERTY(EditDefaultsOnly)
	float OrbitRadius = 100.f;

	UPROPERTY(EditDefaultsOnly)
	float OrbitSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly)
	float MissileLifeSpan = 10.f;
	
	UPROPERTY(EditDefaultsOnly)
	float InitialDelayDuration = 1.5f;
};
