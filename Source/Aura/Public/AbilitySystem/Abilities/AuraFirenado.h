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

	virtual void CheckAbilityUpgrades(FGameplayTag AbilityTag) override;
	
	UFUNCTION(BlueprintCallable)
	AAuraFireTornado* SpawnTornado();

	UFUNCTION(BlueprintCallable)
	AAuraFireTornado* SpawnTornadoToLocation(const FVector& Location);
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraFireTornado> FireTornadoClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float DamageDeltaSecond = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float SpawnTime = 5.f;
	
	UPROPERTY(EditDefaultsOnly)
	float FollowRadius = 600.f;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageRadius = 300.f;
};
