// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SpellUpgradesWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Player/AuraPlayerState.h"

void USpellUpgradesWidgetController::BroadcastInitialValues()
{
	UpgradeInfo.Empty();
	
	// PlayerState로부터 업그레이드 배열 받아오기
	FOwnedAbilityUpgradeList& List = GetAuraPS()->GetOwnedAbilityUpgradeList();
	
	for (auto Upgrade : List.OwnedAbilityUpgrades)
	{
		FGameplayTag UpgradeTag = Upgrade.UpgradeTag;
		int32 Stack = Upgrade.UpgradeStack;

		auto InfoStruct = UAuraAbilitySystemLibrary::GetAbilityUpgradeInfoForUpgradeTag(this, UpgradeTag);
		FString UpgradeName = InfoStruct.UpgradeName;
		
		UpgradeInfo.Add(UpgradeName, Stack);
	}
}

void USpellUpgradesWidgetController::BindCallbacksToDependencies()
{

}