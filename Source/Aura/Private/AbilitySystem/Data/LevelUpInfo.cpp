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
