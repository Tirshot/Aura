// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraMindControl.generated.h"

UCLASS()
class AURA_API UAuraMindControl : public UAuraGameplayAbility
{
	GENERATED_BODY()

public:
	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject) override;
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject) override;

public:
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Gameplay Ability")
	FScalableFloat MindControlDuration;

	UPROPERTY(BlueprintReadWrite)
	AActor* ControlledActor;
	
	UPROPERTY(BlueprintReadWrite)
	FName OriginalTag;
};
