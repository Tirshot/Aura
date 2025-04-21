// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AuraGameplayAbility.generated.h"

class UAuraAbilitySystemComponent;

UCLASS()
class AURA_API UAuraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	// 시작 시 부여되는 입력 태그
	UPROPERTY(EditDefaultsOnly, Category="Input")
	FGameplayTag StartupInputTag;

	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject);
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject);

	static FString GetLockedDescription(int32 Level, int32 InferiorAbilityLevel = 0);

protected:

	float GetManaCost(float InLevel = 1.f) const;
	float GetCoolDown(float InLevel = 1.f) const;
};
