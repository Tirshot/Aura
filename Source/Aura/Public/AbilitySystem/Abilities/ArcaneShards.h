// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "ArcaneShards.generated.h"

UCLASS()
class AURA_API UArcaneShards : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()
	
public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;

public:
	virtual bool CheckAbilityUpgrades(FGameplayTag AbilityTag) override;

private:
	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess = true))
	int32 NumPoints = 0;
};
