// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/MissionInfo.h"

#include "Game/AuraGameStateBase.h"

void FMissionData::PostReplicatedAdd(const FMissionDataArray& InArraySerializer)
{
	if (InArraySerializer.OwningGameState)
	{
		InArraySerializer.OwningGameState->BroadcastMissionData();
	}
}

void FMissionData::PostReplicatedChange(const FMissionDataArray& InArraySerializer)
{
	if (InArraySerializer.OwningGameState)
	{
		InArraySerializer.OwningGameState->BroadcastMissionData();
	}
}