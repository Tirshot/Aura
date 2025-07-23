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
	
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;

	UFUNCTION(BlueprintCallable)
	TArray<AAuraFireBall*> SpawnFireBalls();

public:
	virtual void CheckAbilityUpgrades(FGameplayTag AbilityTag) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "FireBlast")
	int32 BaseNumFireBalls = 12;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FireBlast")
	int32 NumFireBalls = 0;

private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraFireBall> FireBallClass;
};
