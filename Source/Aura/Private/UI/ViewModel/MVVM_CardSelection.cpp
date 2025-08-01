// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_CardSelection.h"

#include "AuraGameplayTags.h"
#include "UI/ViewModel/MVVM_AbilityCard.h"

void UMVVM_CardSelection::InitializeSlot()
{
	Card_0 = NewObject<UMVVM_AbilityCard>(this, AbilityCardViewModelClass);
	Card_0->CardIndex = 0;
	// Card_0->SetUpgradeTag(FAuraGameplayTags::Get().Abilities_None);
	
	Card_1 = NewObject<UMVVM_AbilityCard>(this, AbilityCardViewModelClass);
	Card_1->CardIndex = 1;
	// Card_1->SetUpgradeTag(FAuraGameplayTags::Get().Abilities_None);

	Card_2 = NewObject<UMVVM_AbilityCard>(this, AbilityCardViewModelClass);
	Card_2->CardIndex = 2;
	// Card_2->SetUpgradeTag(FAuraGameplayTags::Get().Abilities_None);

	AbilityCards.Add(0, Card_0);
	AbilityCards.Add(1, Card_1);
	AbilityCards.Add(2, Card_2);
}

UMVVM_AbilityCard* UMVVM_CardSelection::GetCardViewModelByIndex(int32 Index)
{
	return AbilityCards.FindChecked(Index);
}

void UMVVM_CardSelection::RerollButtonClicked()
{
	OnRerollSelectedDelegate.Broadcast();
}
