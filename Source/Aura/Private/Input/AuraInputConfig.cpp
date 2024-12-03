// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/AuraInputConfig.h"
#include "InputMappingContext.h"

const UInputAction* UAuraInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (auto& InputAction : AbilityInputActions)
	{
		if (InputAction.InputAction && InputAction.InputTag.MatchesTagExact(InputTag))
		{
			return InputAction.InputAction;
		}
	}

	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("InputConfig [%s]에서 InputTag [%s]를 가지는 Ability Input Action을 찾을 수 없음"), *GetNameSafe(this), *InputTag.ToString());
	}

	return nullptr;
}
