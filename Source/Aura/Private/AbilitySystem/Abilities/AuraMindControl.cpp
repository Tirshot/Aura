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

void UAuraMindControl::SetOriginalTag(AActor* TargetActor)
{
	OriginalTag = TargetActor->Tags[0];
}

bool UAuraMindControl::ChangeActorTag(AActor* TargetActor, EActorTag ActorTag)
{
	if (!TargetActor)
		return false;
	
	if (TargetActor->ActorHasTag(FName("NotMindControllable")))
	{
		return false;
	}

	if (TargetActor->Tags.IsEmpty())
		return false;

	switch (ActorTag)
	{
	case Player:
		if (OriginalTag == FName("Player"))
		{
			TargetActor->Tags[0] = OriginalTag;
			return false;
		}
		
		TargetActor->Tags.Remove(FName("Enemy"));
		TargetActor->Tags.AddUnique(FName("Player"));
		return true;

	case Enemy:
		if (OriginalTag == FName("Enemy"))
		{
			TargetActor->Tags[0] = OriginalTag;
			return false;
		}
		
		TargetActor->Tags.Remove(FName("Player"));
		TargetActor->Tags.AddUnique(FName("Enemy"));
		return true;
	}
	
	return false;
}

float UAuraMindControl::GetScalableFloatToFloat(const FScalableFloat ScalableFloat)
{
	return ScalableFloat.GetValueAtLevel(GetAbilityLevel());
}
