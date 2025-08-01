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

	virtual void CheckAbilityUpgrades() override;
	
	UFUNCTION(BlueprintCallable)
	AAuraElectroSphere* SpawnElectroSphere(const FVector& Location);
	
	UFUNCTION(BlueprintImplementableEvent)
	void AdditionalTargetDied(AActor* DeadActor);
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraElectroSphere> ElectroSphereClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
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
	int32 AdditionalTargets = 2;
	
	UPROPERTY(EditDefaultsOnly)
	int32 MaxNumShockTargets = 6;
};
