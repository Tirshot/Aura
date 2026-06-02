// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraMindControl.h"

#include "AbilitySystem/AbilityTasks/TargetDataUnderMouse.h"


FString UAuraMindControl::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	return Super::GetDescription(Level, WorldContextObject);
}

FString UAuraMindControl::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	return Super::GetNextLevelDescription(Level, WorldContextObject);
}
