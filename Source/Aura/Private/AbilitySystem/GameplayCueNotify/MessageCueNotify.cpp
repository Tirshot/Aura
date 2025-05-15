// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayCueNotify/MessageCueNotify.h"

#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"

void UMessageCueNotify::HandleGameplayCue(AActor* MyTarget, EGameplayCueEvent::Type EventType,
                                          const FGameplayCueParameters& Parameters)
{
	// 작동된 이후에만 호출
	// if (EventType != EGameplayCueEvent::Executed)
	// 	return;

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyTarget);
	if (ASC == nullptr)
		return;

	UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(ASC);
	if (AuraASC == nullptr)
		return;

	FGameplayTag Tag = Parameters.OriginalTag;
	
	if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag("GameplayCue.Message")))
	{
		AuraASC->OnMessageTagReceived.Broadcast(Parameters.OriginalTag);
	}
}