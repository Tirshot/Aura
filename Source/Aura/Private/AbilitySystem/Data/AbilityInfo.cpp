// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/AbilityInfo.h"
#include "AuraLogChannels.h"

FAuraAbilityInfo* UAbilityInfo::FindAbilityInfoForTag(const FGameplayTag& AbilityTag, bool bLogNotFound)
{
	for (FAuraAbilityInfo& Info : AbilityInformation)
	{
		if (Info.AbilityTag == AbilityTag)
		{
			return &Info;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogAura, Error, TEXT("Can't Find Info for AbilityTag [%s] on AbilityInfo [%s]"), *AbilityTag.ToString(), *GetNameSafe(this));
	}

	return nullptr;
}

FName UAbilityInfo::GetAbilityNameForTag(const FGameplayTag& AbilityTag)
{
	for (const FAuraAbilityInfo& Info : AbilityInformation)
	{
		if (Info.AbilityTag == AbilityTag)
		{
			return Info.AbilityName;
		}
	}

	return FName();
}

const UTexture2D* UAbilityInfo::GetIconForTag(const FGameplayTag& AbilityTag)
{
	for (auto Ability : AbilityInformation)
	{
		if (Ability.AbilityTag == AbilityTag)
		{
			return Ability.Icon.Get();
		}
	}
	return nullptr;
}
