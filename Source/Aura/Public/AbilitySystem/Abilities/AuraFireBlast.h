// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraProjectileSpell.h"
#include "AuraFireBlast.generated.h"

class AAuraFireBall;

UCLASS()
class AURA_API UAuraFireBlast : public UAuraProjectileSpell
{
	GENERATED_BODY()
	
public:
	UAuraFireBlast();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;
	
	UFUNCTION(BlueprintCallable)
	TArray<AAuraFireBall*> SpawnFireBalls();

public:
	virtual void CheckAbilityUpgrades() override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FireBlast")
	int32 BaseNumFireBalls = 12;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireBlast")
	int32 NumFireBalls = 0;

	UPROPERTY(EditDefaultsOnly)
	bool bSpawnFireBallDelayed = false;
	
	UPROPERTY(EditDefaultsOnly)
	float ShardFireBallDelay = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bShowBlastIndicator = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bExplodeAtMaxRange = false;
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraFireBall> FireBallClass;
};
