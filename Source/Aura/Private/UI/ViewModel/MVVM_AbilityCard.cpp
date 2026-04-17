// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_AbilityCard.h"

#include "UI/ViewModel/MVVM_CardSelection.h"

void UMVVM_AbilityCard::SetUpgradeTag(FGameplayTag InGameplayTag)
{
	UE_MVVM_SET_PROPERTY_VALUE(UpgradeTag, InGameplayTag);
}

void UMVVM_AbilityCard::SetUpgradeName(FString InUpgradeName)
{
	UE_MVVM_SET_PROPERTY_VALUE(UpgradeName, InUpgradeName);
}

void UMVVM_AbilityCard::SetUpgradeDescription(FText InUpgradeDescription)
{
	UE_MVVM_SET_PROPERTY_VALUE(UpgradeDescription, InUpgradeDescription);
}

void UMVVM_AbilityCard::SetMaxStack(int32 InMaxStack)
{
	UE_MVVM_SET_PROPERTY_VALUE(MaxStack, InMaxStack);
}

void UMVVM_AbilityCard::SetUpgradeRarity(EUpgradeRarity InRarity)
{
	UpgradeRarity = InRarity;

	switch (InRarity)
	{
	case EUpgradeRarity::Common:
		SetUpgradeRarityString("Common");
		break;

	case EUpgradeRarity::Rare:
		SetUpgradeRarityString("Rare");
		break;

	case EUpgradeRarity::Unique:
		SetUpgradeRarityString("Unique");
		break;

	case EUpgradeRarity::Legendary:
		SetUpgradeRarityString("Legendary");
		break;
	}
}

void UMVVM_AbilityCard::SetUpgradeRarityString(FString InRarity)
{
	UE_MVVM_SET_PROPERTY_VALUE(UpgradeRarityString, InRarity);
}

void UMVVM_AbilityCard::UpgradeButtonClicked()
{
	// 바인딩은 플레이어 컨트롤러에서!!
	// OnUpgradeSelectedDelegate.Broadcast(UpgradeTag);
	
	// 부모 뷰 모델에게 알림
	if (!CardSelectionViewModel)
		return;
	
	CardSelectionViewModel->OnUpgradeSelectedOnCardDelegate.Broadcast(UpgradeTag);
}
