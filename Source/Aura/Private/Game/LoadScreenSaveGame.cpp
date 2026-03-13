// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/LoadScreenSaveGame.h"

void ULoadScreenSaveGame::AddOneTimeUseActor(FGuid Guid, bool bReached)
{
	OneTimeUseActors.Add(Guid, bReached);
}

bool ULoadScreenSaveGame::IsUsedActor(FGuid Guid)
{
	bool* bIsUsed = OneTimeUseActors.Find(Guid);
	return bIsUsed ? *bIsUsed : false;
}

FSavedMap ULoadScreenSaveGame::GetSavedMapWithMapName(const FString& InMapName)
{
	for (const FSavedMap& Map : SavedMaps)
	{
		if (Map.MapAssetName == InMapName)
			return Map;
	}
	return FSavedMap();
}

bool ULoadScreenSaveGame::HasMap(const FString& InMapName)
{
	for (const FSavedMap& Map : SavedMaps)
	{
		if (Map.MapAssetName == InMapName)
			return true;
	}
	return false;
}
