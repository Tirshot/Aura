// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_AbilityCard.h"

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

void UMVVM_AbilityCard::SetUpgradeMaxLevel(int32 InUpgradeMaxLevel)
{
	UE_MVVM_SET_PROPERTY_VALUE(UpgradeMaxLevel, InUpgradeMaxLevel);
}

void UMVVM_AbilityCard::UpgradeButtonClicked()
{
	// 바인딩은 플레이어 컨트롤러에서!!
	OnUpgradeSelectedDelegate.Broadcast(UpgradeTag);
}