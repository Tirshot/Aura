// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/LevelUpInfo.h"

int32 ULevelUpInfo::FindLevelForXP(int32 XP) const
{
	// ����ġ �ʿ䷮ ã��
	for (int i = 0; i < LevelUpInformation.Num(); i++)
	{
		if (LevelUpInformation[i].LevelUpRequirement >= XP)
		{
			return i;
		}
	}

	return 1;
}

int32 ULevelUpInfo::FindXPForLevel(int32 Level) const
{
	int32 MaxLevel = LevelUpInformation.Num() - 1;
	
	if (MaxLevel >= Level)
		return LevelUpInformation[Level].LevelUpRequirement;

	return 0;
}
