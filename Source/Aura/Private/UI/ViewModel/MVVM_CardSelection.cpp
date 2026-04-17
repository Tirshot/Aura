// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_CardSelection.h"

#include "UI/ViewModel/MVVM_AbilityCard.h"

void UMVVM_CardSelection::InitializeSlot()
{
	Card_0 = NewObject<UMVVM_AbilityCard>(this, AbilityCardViewModelClass);
	Card_0->CardIndex = 0;
	Card_0->CardSelectionViewModel = this;
	
	Card_1 = NewObject<UMVVM_AbilityCard>(this, AbilityCardViewModelClass);
	Card_1->CardIndex = 1;
	Card_1->CardSelectionViewModel = this;

	Card_2 = NewObject<UMVVM_AbilityCard>(this, AbilityCardViewModelClass);
	Card_2->CardIndex = 2;
	Card_2->CardSelectionViewModel = this;

	AbilityCards.Add(0, Card_0);
	AbilityCards.Add(1, Card_1);
	AbilityCards.Add(2, Card_2);
	
	OnCloseSelectedDelegate.AddDynamic(this, &UMVVM_CardSelection::OnCloseSelected);
}

UMVVM_AbilityCard* UMVVM_CardSelection::GetCardViewModelByIndex(int32 Index)
{
	return AbilityCards.FindChecked(Index);
}

void UMVVM_CardSelection::RerollButtonClicked()
{
	OnRerollSelectedDelegate.Broadcast();
}

void UMVVM_CardSelection::OnCloseSelected()
{
	// 빈 태그 전달
	OnUpgradeSelectedOnCardDelegate.Broadcast(FGameplayTag::EmptyTag);
}
