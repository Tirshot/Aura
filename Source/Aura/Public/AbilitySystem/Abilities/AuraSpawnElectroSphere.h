// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AuraSpawnElectroSphere.generated.h"

class AAuraElectroSphere;

UCLASS()
class AURA_API UAuraSpawnElectroSphere : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()
	
public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject);
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject);


protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	virtual void CheckAbilityUpgrades() override;

protected:
	// 메인 로직
	UFUNCTION(BlueprintCallable)
	AAuraElectroSphere* SpawnElectroSphere(const FVector& Location);
	
	UFUNCTION(BlueprintImplementableEvent)
	void AdditionalTargetDied(AActor* DeadActor);
	
protected:
	void ShowMagicCircleAndRangeIndicator();

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraElectroSphere> ElectroSphereClass;
	
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TObjectPtr<AAuraElectroSphere> ElectroSphere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float DamageDeltaSecond = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float SpawnTime = 5.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MovementSpeed = 500.f;
	
	UPROPERTY(EditDefaultsOnly)
	bool bFollowTarget = false;
	
	UPROPERTY(EditDefaultsOnly)
	float TraceRadius = 500.f;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageRadius = 300.f;
	
	UPROPERTY(EditDefaultsOnly)
	int32 NumAdditionalTargets = 2;
};
