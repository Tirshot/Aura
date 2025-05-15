// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraDamageGameplayAbility.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "ElectroTeleport.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UElectroTeleport : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

public:
	UElectroTeleport();
	
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;

public:
	UFUNCTION(BlueprintCallable)
	bool TeleportToLocation(const FHitResult& HitResult);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float MaxTeleportDistance = 500.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float MaxHeight = 500.f;
};
