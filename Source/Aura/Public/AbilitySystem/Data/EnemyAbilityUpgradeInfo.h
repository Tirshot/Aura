// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Engine/DataAsset.h"
#include "EnemyAbilityUpgradeInfo.generated.h"

USTRUCT(BlueprintType)
struct FEnemyAbilityUpgradeInfoStruct
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FString UpgradeTitle;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EnemyAbilityUpgrade;
	
	UPROPERTY(EditDefaultsOnly, meta=(MultiLine = true))
	FString UpgradeDescription;
};


UCLASS()
class AURA_API UEnemyAbilityUpgradeInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	FEnemyAbilityUpgradeInfoStruct GetEnemyAbilityUpgradeInfoByTag(const FGameplayTag& Tag);

	UFUNCTION(BlueprintCallable)
	FEnemyAbilityUpgradeInfoStruct GetEnemyAbilityUpgradeInfoByClass(TSubclassOf<UGameplayEffect> EnemyAbilityUpgrade);


public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FEnemyAbilityUpgradeInfoStruct> EnemyAbilityUpgradeInfos;
};
