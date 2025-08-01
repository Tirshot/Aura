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
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void CheckAbilityUpgrades() override;
	
public:
	UFUNCTION(BlueprintCallable)
	bool TeleportToLocation(const FVector& FromLocation, const FVector& ToLocation);

	UFUNCTION(BlueprintCallable)
	bool ReturnToInitialLocation();

	// 잔상 효과
	UFUNCTION(BlueprintCallable)
	void GhostEffect(TSubclassOf<AGhostEffectActor> GhostClass, UMaterialInterface* GhostMaterial);

	UFUNCTION(BlueprintCallable)
	bool ShouldTeleportCooldownReset();
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float MaxTeleportDistance = 500.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float MaxHeight = 500.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Teleport")
	FVector InitialLocation = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Teleport")
	FVector DestinedLocation = FVector::ZeroVector;
	
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	int NumGhosts = 5;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Teleport")
	float GhostLifeSpan = 0.5f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Teleport|Upgrades")
	float TeleportCooldownResetProbability = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Teleport|Upgrades")
	bool bCanReturn = false;
};
