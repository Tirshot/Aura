// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "AbilityUpgradeInfo.generated.h"

UENUM(BlueprintType)
enum class EUpgradeRarity : uint8
{
	Common = 0,
	Rare,
	Unique,
	Legendary,
};

USTRUCT(BlueprintType)
struct FAuraAbilityUpgradeInfo
{
	GENERATED_BODY();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FString UpgradeName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(MultiLine = true))
	FText UpgradeDescription;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EUpgradeRarity Rarity = EUpgradeRarity::Common;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 UpgradeMaxLevel = 1;

	// 실제 적용할 효과 타입 (ex. 데미지 +, 쿨다운 감소 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag UpgradeEffectTag;

	bool operator==(const FAuraAbilityUpgradeInfo& Other) const
	{
		return UpgradeEffectTag == Other.UpgradeEffectTag;
	}
};

// 배열로 래핑하기 위한 구조체
USTRUCT(BlueprintType)
struct AURA_API FAuraAbilityUpgradeInfoArray
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade Info Array")
	TArray<FAuraAbilityUpgradeInfo> UpgradeInfos;

	TArray<FAuraAbilityUpgradeInfo>& GetUpgradeInfoByArray() { return UpgradeInfos; }
};


UCLASS()
class AURA_API UAbilityUpgradeInfo : public UDataAsset
{
	GENERATED_BODY()

protected:
	virtual void PostLoad() override;
	void RebuildUpgradeInfoMaps();
	
public:
#if WITH_EDITOR
	// 에디터에서 변경될 때 호출
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
public:
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FAuraAbilityUpgradeInfoArray GetUpgradesForAbility(const FGameplayTag& AbilityTag);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FAuraAbilityUpgradeInfo GetUpgradeInfoForUpgradeTag(const FGameplayTag& UpgradeTag);

	UFUNCTION()
	FGameplayTag GetRandomUpgradeTagForAbility(const FGameplayTag& AbilityTag);

	UFUNCTION()
	TArray<FAuraAbilityUpgradeInfo> GetAvailableUpgradeInfo(const TArray<FAuraAbilityUpgradeInfo>& InUpgradeInfos, EUpgradeRarity InRarity);

	UFUNCTION()
	TArray<FAuraAbilityUpgradeInfo> GetAvailableUpgradeInfoForTag(const TArray<FGameplayTag>& InUpgradeTags, EUpgradeRarity InRarity);

	UFUNCTION(BlueprintCallable)
	float GetProbabilityForUpgradeTag(const FGameplayTag& UpgradeTag);

	// 레어도에 따른 업그레이드 배열
	UFUNCTION()
	TArray<FAuraAbilityUpgradeInfo> GetUpgradeInfoArrayForProbability(EUpgradeRarity Rarity);

	UFUNCTION(BlueprintCallable)
	FGameplayTag GetAbilityTagForUpgradeTag(FGameplayTag UpgradeTag);

	// 각 레어도에 따른 확률
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<EUpgradeRarity, float> UpgradeProbability;
	
	// 어빌리티 태그, 업그레이드 정보의 배열 키-값 쌍
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FAuraAbilityUpgradeInfoArray> AbilityUpgrades;

	// 업그레이드 태그, 업그레이드 정보 키-값 쌍 
	UPROPERTY()
	TMap<FGameplayTag, FAuraAbilityUpgradeInfo> UpgradeInfosByEffectTag;
};
