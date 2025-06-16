// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AbilityUpgradeInfo.generated.h"

USTRUCT(BlueprintType)
struct FAuraAbilityUpgradeInfo
{
	GENERATED_BODY();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString UpgradeName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString UpgradeDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 UpgradeMaxLevel = 1;

	// 실제 적용할 효과 타입 (ex. 데미지 +, 쿨다운 감소 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag UpgradeEffectTag;
};

// 배열로 래핑하기 위한 구조체
USTRUCT(BlueprintType)
struct AURA_API FAuraAbilityUpgradeInfoArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Info Array")
	TArray<FAuraAbilityUpgradeInfo> UpgradeInfos;
};


UCLASS()
class AURA_API UAbilityUpgradeInfo : public UDataAsset
{
	GENERATED_BODY()

protected:
	virtual void PostLoad() override;
	
public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FAuraAbilityUpgradeInfoArray GetUpgradesForAbility(const FGameplayTag& AbilityTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FAuraAbilityUpgradeInfo GetUpgradeInfoForUpgradeTag(const FGameplayTag& UpgradeTag);

	UFUNCTION()
	FGameplayTag GetRandomUpgradeTagForAbility(const FGameplayTag& AbilityTag);
	
	// 어빌리티 태그, 업그레이드 정보의 배열 키-값 쌍
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FAuraAbilityUpgradeInfoArray> AbilityUpgrades;

	// 업그레이드 태그, 업그레이드 정보 키-값 쌍 
	UPROPERTY(Transient) // 런타임에만 생성됨
	TMap<FGameplayTag, FAuraAbilityUpgradeInfo> UpgradeInfosByEffectTag;
};
