// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AuraArcaneAreaAbility.generated.h"

class AAuraArcaneArea;

UCLASS()
class AURA_API UAuraArcaneAreaAbility : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;

	virtual void CheckAbilityUpgrades() override;

protected:
	UFUNCTION(BlueprintCallable)
	void SpawnArcaneArea(const FVector& Location);

	UFUNCTION()
	void OnArcaneAreaDestroyed(AActor* DestroyedActor);
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="ArcaneArea")
	bool bTakeDamage = false;
	
	UPROPERTY(EditDefaultsOnly, Category="ArcaneArea")
	float LifeSpan = 5.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ArcaneArea")
	float SlowDownRatio = 0.25f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ArcaneArea")
	float SlowRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category="ArcaneArea")
	float ApplyEffectPeriod = 0.2f;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraArcaneArea> ArcaneAreaClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AAuraArcaneArea> ArcaneArea;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> SlowDownEffectClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> SlowDownDecayEffectClass;
};
