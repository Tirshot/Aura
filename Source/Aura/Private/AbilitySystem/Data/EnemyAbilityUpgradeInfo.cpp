// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/EnemyAbilityUpgradeInfo.h"

FEnemyAbilityUpgradeInfoStruct UEnemyAbilityUpgradeInfo::GetEnemyAbilityUpgradeInfoByTag(const FGameplayTag& Tag)
{
	for (auto& UpgradeInfo : EnemyAbilityUpgradeInfos)
	{
		// 클래스 디폴트 오브젝트 가져오기
		const UGameplayEffect* CDO = GetDefault<UGameplayEffect>(UpgradeInfo.EnemyAbilityUpgrade);
		if (CDO)
		{
			// Granted Tag :: 액터에게 부여하는 태그
			auto& GrantedTags= CDO->GetGrantedTags();
			if (GrantedTags.HasTag(Tag))
			{
				return UpgradeInfo;
			}
		}
	}
	return FEnemyAbilityUpgradeInfoStruct();
}

FEnemyAbilityUpgradeInfoStruct UEnemyAbilityUpgradeInfo::GetEnemyAbilityUpgradeInfoByClass(
	TSubclassOf<UGameplayEffect> EnemyAbilityUpgrade)
{
	for (auto& UpgradeInfo : EnemyAbilityUpgradeInfos)
	{
		if (UpgradeInfo.EnemyAbilityUpgrade == EnemyAbilityUpgrade)
		{
			return UpgradeInfo;
		}
	}
	return FEnemyAbilityUpgradeInfoStruct();
}
