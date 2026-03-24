// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/SoundData.h"

USoundBase* USoundData::GetSoundByTag(FGameplayTag Tag)
{
	for (const auto& SoundContext : SoundContexts)
	{
		if (SoundContext.SoundTag.MatchesTag(Tag))
		{
			return SoundContext.SoundAsset;
		}
	}
	return nullptr;
}

FSoundContext USoundData::GetSoundContextByTag(FGameplayTag Tag)
{
	for (auto& SoundContext : SoundContexts)
	{
		if (SoundContext.SoundTag.MatchesTag(Tag))
		{
			return SoundContext;
		}
	}
	
	return FSoundContext();
}
