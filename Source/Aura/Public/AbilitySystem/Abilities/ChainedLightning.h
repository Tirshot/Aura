// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraProjectileSpell.h"
#include "ChainedLightning.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UChainedLightning : public UAuraProjectileSpell
{
	GENERATED_BODY()
	
public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	virtual void CheckAbilityUpgrades() override;
	
	virtual void SpawnProjectile(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag, bool bOverridePitch = false, float PitchOverride = 0) override;
	
public:
	
protected:
	// 반사 반경
	UPROPERTY()
	float DefaultChainRadius = 500.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Chained Lightning")
	float ChainRadius = 500.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Chained Lightning")
	float DamageReductionRatio = 0.667f;
	
	// 반사 횟수
	UPROPERTY()
	int32 DefaultChain = 5;
	
	// 반사 횟수
	UPROPERTY(EditDefaultsOnly, Category="Chained Lightning")
	int32 MaxChain = 5;
	
};
