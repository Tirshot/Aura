// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SpellUpgradesWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"

void USpellUpgradesWidgetController::BroadcastInitialValues()
{
	UpgradeInfo.Empty();
	
	// PlayerState로부터 업그레이드 배열 받아오기
	for (auto& Pair :GetAuraPS()->GetAbilityUpgradeTagContainer())
	{
		FGameplayTag UpgradeTag = Pair.Key;
		int32 Stack = Pair.Value;

		auto InfoStruct = UAuraAbilitySystemLibrary::GetAbilityUpgradeInfoForUpgradeTag(this, UpgradeTag);
		FString UpgradeName = InfoStruct.UpgradeName;
		
		UpgradeInfo.Add(UpgradeName, Stack);
	}
}

void USpellUpgradesWidgetController::BindCallbacksToDependencies()
{
	// 업그레이드 태그 컨테이너 변화
	//GetAuraPS()->OnAbilityUpgradeTagsChangedDelegate.AddDynamic(this, &USpellUpgradesWidgetController::RecievedUpgradeTags);
}

void USpellUpgradesWidgetController::ReceivedUpgradeTags(FGameplayTag UpgradeTag, int32 Stack)
{
	// 위젯으로 메시지 전송
	//OnReceivedUpgradeTags.Broadcast(UpgradeTag, Stack);
}