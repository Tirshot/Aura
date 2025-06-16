// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraDamageGameplayAbility.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "ElectroTeleport.generated.h"

class AGhostEffectActor;
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

	virtual bool CheckAbilityUpgrades(FGameplayTag AbilityTag) override;
	
public:
	UFUNCTION(BlueprintCallable)
	bool TeleportToLocation(const FHitResult& HitResult);

	// 잔상 효과
	UFUNCTION(BlueprintCallable)
	void GhostEffect(const FVector& InitialLocation, const FVector& DestinationLocation, TSubclassOf<AGhostEffectActor> GhostClass, UMaterialInterface* GhostMaterial);

	UFUNCTION(BlueprintCallable)
	bool ShouldTeleportCooldownReset();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float MaxTeleportDistance = 500.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float MaxHeight = 500.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	int NumGhosts = 5;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float GhostLifeSpan = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Upgrades")
	float TeleportCooldownResetProbability = 0.f;
};
