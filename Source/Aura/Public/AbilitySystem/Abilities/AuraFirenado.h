// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AuraFirenado.generated.h"

class AAuraFireTornado;

UCLASS()
class AURA_API UAuraFirenado : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject);
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject);

	virtual void CheckAbilityUpgrades() override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	UFUNCTION(BlueprintCallable)
	void CalculateRange();
	
	UFUNCTION(BlueprintCallable)
	AAuraFireTornado* SpawnTornadoToLocation(const FVector& Location);

	UFUNCTION(BlueprintCallable)
	void StoreMouseLocation();

	UFUNCTION(BlueprintCallable)
	void DestroyTornadoAndCommitCooldownEndAbility();
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraFireTornado> FireTornadoClass;

	UPROPERTY()
	TObjectPtr<AAuraFireTornado> FireTornado;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> DestroySound;
	
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	FVector MouseLocation = FVector::ZeroVector;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float DamageDeltaSecond = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float SpawnTime = 5.f;
	
	UPROPERTY(EditDefaultsOnly)
	float FollowRadius = 600.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float DamageRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float RangePerLevel = 80.f;

	UPROPERTY()
	FTimerHandle DestroyTimer;
};
