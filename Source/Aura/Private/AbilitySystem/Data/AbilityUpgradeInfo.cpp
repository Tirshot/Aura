// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/AbilityUpgradeInfo.h"

#include "AuraGameplayTags.h"
#include "Kismet/KismetMathLibrary.h"

void UAbilityUpgradeInfo::PostLoad()
{
	Super::PostLoad();

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

FAuraAbilityUpgradeInfoArray UAbilityUpgradeInfo::GetUpgradesForAbility(const FGameplayTag& AbilityTag)
{
	if (FAuraAbilityUpgradeInfoArray* FoundInfoArray = AbilityUpgrades.Find(AbilityTag))
	{
		return *FoundInfoArray;
	}
	return FAuraAbilityUpgradeInfoArray();
}

FAuraAbilityUpgradeInfo UAbilityUpgradeInfo::GetUpgradeInfoForUpgradeTag(const FGameplayTag& UpgradeTag)
{
	if (!UpgradeTag.IsValid())
		return FAuraAbilityUpgradeInfo();
	
	// return UpgradeInfosByEffectTag.FindChecked(UpgradeTag);

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
	auto AuraTags = FAuraGameplayTags::Get();
	
	FAuraAbilityUpgradeInfoArray UpgradeInfoArray = GetUpgradesForAbility(AbilityTag);
	if (UpgradeInfoArray.UpgradeInfos.IsEmpty())
		return AuraTags.Abilities_None;

	int32 ArrayNum = UpgradeInfoArray.UpgradeInfos.Num();
	int32 RandValue = UKismetMathLibrary::RandomIntegerInRange(0, ArrayNum - 1);

	return UpgradeInfoArray.UpgradeInfos[RandValue].UpgradeEffectTag;
}
