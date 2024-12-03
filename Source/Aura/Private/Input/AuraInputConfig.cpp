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
		UE_LOG(LogTemp, Error, TEXT("InputConfig [%s]���� InputTag [%s]�� ������ Ability Input Action�� ã�� �� ����"), *GetNameSafe(this), *InputTag.ToString());
	}

	return nullptr;
}
