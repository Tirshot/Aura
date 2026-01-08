// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/ItemInfo.h"

FItemData UItemInfo::GetItemDataByID_Copy(const FName& ItemID) const
{
	// 블루프린트 용 
	if (const FItemData* Found = ItemTable->FindRow<FItemData>(ItemID, TEXT("FoundRow")))
	{
		return *Found;
	}
	return FItemData();
}

const FItemData* UItemInfo::GetItemDataByID(const FName& ItemID) const
{
	return ItemTable->FindRow<FItemData>(ItemID, TEXT("FoundRow"));
}

const FDropItemGroupArray* UItemInfo::GetDropItemGroup(ECharacterClass EnemyClass)
{
	for (const auto& Pair : DropList)
	{
		if (Pair.Key == EnemyClass)
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}