// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/LevelUpInfo.h"

int32 ULevelUpInfo::FindLevelForXP(int32 XP) const
{
	// 경험치 필요량 찾기
	for (int i = 0; i < LevelUpInformation.Num(); i++)
	{
		if (LevelUpInformation[i].LevelUpRequirement >= XP)
		{
			return i;
		}
	}

	return 1;
}
