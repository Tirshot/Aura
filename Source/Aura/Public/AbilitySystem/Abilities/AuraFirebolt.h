// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraProjectileSpell.h"
#include "AuraFirebolt.generated.h"

UCLASS()
class AURA_API UAuraFirebolt : public UAuraProjectileSpell
{
	GENERATED_BODY()

public:
	UAuraFirebolt();
	
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;

	// ���� ����ü
	UFUNCTION(BlueprintCallable)
	void SpawnProjectiles(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag, bool bOverridePitch, float PitchOverride, bool bNumProjectileOverride, int32 NumProjectileOverride, AActor* HomingTarget);

	virtual void CheckAbilityUpgrades() override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 NumProjectiles = 1;
	
	UPROPERTY(EditDefaultsOnly, Category = "Firebolt")
	float ProjectileSpread = 90.f;

	UPROPERTY(EditDefaultsOnly, Category = "Firebolt")
	float HomingAccMin = 1600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Firebolt")
	float HomingAccMax = 3200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Firebolt")
	bool bLaunchHomingProjectile = true;
	
	int32 SpeedUpCount = 0;
};
