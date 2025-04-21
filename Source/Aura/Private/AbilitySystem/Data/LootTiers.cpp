// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/LootTiers.h"

TArray<FLootItem> ULootTiers::GetLootItems()
{
	TArray<FLootItem> ReturnItems;

	for (auto& Item : LootItems)
	{
		// 각 아이템의 최대 드랍 개수
		for (int32 i = 0; i < Item.MaxNumberToSpawn; ++i)
		{
			// 확률 계산
			if (FMath::FRandRange(1.f, 100.f) < Item.ChanceToSpawn)
			{
				// 새로운 아이템을 생성하여 배열에 추가
				FLootItem NewItem;
				NewItem.LootClass = Item.LootClass;
				NewItem.bLootLevelOverride = Item.bLootLevelOverride;
				ReturnItems.Add(NewItem);
			}
		}
	}
	return ReturnItems;
}
