// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/AbilityUpgradeInfo.h"

#include "AuraGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"

void UAbilityUpgradeInfo::PostLoad()
{
	Super::PostLoad();

	RebuildUpgradeInfoMaps();
}

void UAbilityUpgradeInfo::RebuildUpgradeInfoMaps()
{
	// 로드된 이후에 한 번만 호출
	UpgradeInfosByEffectTag.Empty();
	for (const auto& Pair : AbilityUpgrades)
	{
		for (const auto& UpgradeInfo : Pair.Value.UpgradeInfos)
		{
			// UpgradeEffectTag가 유효하다면 캐시 맵에 추가
			if (UpgradeInfo.UpgradeEffectTag.IsValid())
			{
				UpgradeInfosByEffectTag.Add(UpgradeInfo.UpgradeEffectTag, UpgradeInfo);
			}
		}
	}
}

void UAbilityUpgradeInfo::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr)
	{
		RebuildUpgradeInfoMaps();
	}
}

FAuraAbilityUpgradeInfoArray UAbilityUpgradeInfo::GetUpgradesForAbility(const FGameplayTag& AbilityTag)
{
	if (FAuraAbilityUpgradeInfoArray* FoundInfoArray = AbilityUpgrades.Find(AbilityTag))
	{
		return *FoundInfoArray;
	}

	{
		FGameplayTagContainer ParentTags = AbilityTag.GetGameplayTagParents();
		ParentTags.RemoveTag(AbilityTag);

		if (FAuraAbilityUpgradeInfoArray* AnotherInfoArray = AbilityUpgrades.Find(ParentTags.First()))
		{
			return *AnotherInfoArray;
		}
		
	}
	return FAuraAbilityUpgradeInfoArray();
}

FAuraAbilityUpgradeInfo UAbilityUpgradeInfo::GetUpgradeInfoForUpgradeTag(const FGameplayTag& UpgradeTag)
{
	if (!UpgradeTag.IsValid())
		return FAuraAbilityUpgradeInfo();
	
	const FAuraAbilityUpgradeInfo* FoundInfo = UpgradeInfosByEffectTag.Find(UpgradeTag);

	if (FoundInfo)
	{
		return *FoundInfo;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("GetUpgradeInfoForUpgradeTag: Didn't Found UpgradeTag '%s' (Full Name: %s)"),
			*UpgradeTag.ToString(), *UpgradeTag.GetTagName().ToString());

		return FAuraAbilityUpgradeInfo();
	}
}

FGameplayTag UAbilityUpgradeInfo::GetRandomUpgradeTagForAbility(const FGameplayTag& AbilityTag)
{
	auto& AuraTags = FAuraGameplayTags::Get();
	
	FAuraAbilityUpgradeInfoArray UpgradeInfoArray = GetUpgradesForAbility(AbilityTag);
	if (UpgradeInfoArray.UpgradeInfos.IsEmpty())
		return AuraTags.Abilities_None;

	int32 ArrayNum = UpgradeInfoArray.UpgradeInfos.Num();
	int32 RandValue = UKismetMathLibrary::RandomIntegerInRange(0, ArrayNum - 1);

	return UpgradeInfoArray.UpgradeInfos[RandValue].UpgradeEffectTag;
}

TArray<FAuraAbilityUpgradeInfo> UAbilityUpgradeInfo::GetAvailableUpgradeInfo(
	const TArray<FAuraAbilityUpgradeInfo>& InUpgradeInfos, EUpgradeRarity InRarity)
{
	TArray<FAuraAbilityUpgradeInfo> OutInfos;
	OutInfos.Empty();
	
	// 활성화 된 어빌리티 배열을 받았다고 가정
	for (auto Info : InUpgradeInfos)
	{
		if (Info.Rarity == InRarity)
			OutInfos.AddUnique(Info);
	}

	return OutInfos;
}

TArray<FAuraAbilityUpgradeInfo> UAbilityUpgradeInfo::GetAvailableUpgradeInfoForTag(
	const TArray<FGameplayTag>& InUpgradeTags, EUpgradeRarity InRarity)
{
	TArray<FAuraAbilityUpgradeInfo> OutInfos;
	OutInfos.Empty();
	
	// 활성화 된 어빌리티 태그를 받았다고 가정
	for (auto Tag : InUpgradeTags)
	{
		const FAuraAbilityUpgradeInfoArray& AbilityUpgrade = AbilityUpgrades[Tag];
		for (int i = 0; i < AbilityUpgrade.UpgradeInfos.Num(); i++)
		{
			if (AbilityUpgrade.UpgradeInfos[i].Rarity == InRarity)
			{
				OutInfos.AddUnique(AbilityUpgrade.UpgradeInfos[i]);
			}
		}
	}

	return OutInfos;
}

float UAbilityUpgradeInfo::GetProbabilityForUpgradeTag(const FGameplayTag& UpgradeTag)
{
	auto& Info = UpgradeInfosByEffectTag[UpgradeTag];
	return UpgradeProbability[Info.Rarity];
}

TArray<FAuraAbilityUpgradeInfo> UAbilityUpgradeInfo::GetUpgradeInfoArrayForProbability(EUpgradeRarity Rarity)
{
	TArray<FAuraAbilityUpgradeInfo> OutUpgradeInfos;
	OutUpgradeInfos.Empty();

	for (const auto& UpgradeInfo : UpgradeInfosByEffectTag)
	{
		if (UpgradeInfo.Value.Rarity == Rarity)
		{
			OutUpgradeInfos.AddUnique(UpgradeInfo.Value);
		}
	}
	return OutUpgradeInfos;
}
