// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AuraGameplayAbility.generated.h"

struct FAuraAbilityUpgradeInfo;
class UAuraAbilitySystemComponent;

UCLASS()
class AURA_API UAuraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	// 시작 시 부여되는 입력 태그
	UPROPERTY(EditDefaultsOnly, Category="Input")
	FGameplayTag StartupInputTag;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayAbility")
	float AbilityRange = 300.f;

	virtual FString GetDescription(int32 Level, const UObject* WorldContextObject);
	virtual FString GetNextLevelDescription(int32 Level, const UObject* WorldContextObject);

	static FString GetLockedDescription(int32 Level, const UObject* WorldContextObject, const FGameplayTag& InferiorAbilityTag = FGameplayTag());

public:
	UFUNCTION(BlueprintCallable)
	virtual void CheckAbilityUpgrades() {};
	
	UFUNCTION(BlueprintCallable)
	static TArray<FAuraAbilityUpgradeInfo> GetAbilityUpgradeForTag(AActor* AvatarActor, FGameplayTag AbilityTag);
	
	UFUNCTION(BlueprintPure, Category = "GameplayAbility|Upgrades")
	static bool HasUpgradeTag(AActor* AvatarActor, FGameplayTag Tag);

	UFUNCTION(BlueprintPure, Category = "GameplayAbility|Upgrades")
	int32 GetUpgradeStackCount(AActor* AvatarActor,FGameplayTag Tag);
	
protected:
	float GetManaCost(float InLevel = 1.f) const;
	float GetCoolDown(float InLevel = 1.f) const;
};
