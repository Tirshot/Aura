// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameInstance.h"

#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "IDetailTreeNode.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "GameFramework/GameUserSettings.h"

void UAuraGameInstance::Init()
{
	Super::Init();

	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	if (Settings)
	{
		Settings->LoadSettings(false);
		Settings->ApplySettings(false);
	}

	// 모든 Aura Character 대상으로 인벤토리 컴포넌트의 델리게이트 호출
	if (ItemInfos)
	{
		OnInitialized.Broadcast();
		bInit = true;
	}
}

void UAuraGameInstance::Shutdown()
{
	Super::Shutdown();
	
	if (auto AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetFirstLocalPlayerController())))
	{
		FGameplayEffectQuery Query;
		AuraASC->RemoveActiveEffects(Query);
	}
}

void UAuraGameInstance::SetAllVariablesToDefault()
{
	bVisibleNextButton = false;
	bVisibleLevelUpButton = false;
	bAuraInvincible = false;
	bAuraInfiniteMana = false;
}

UItemInfo* UAuraGameInstance::GetItemInfos()
{
	return ItemInfos;
}

const FItemData* UAuraGameInstance::GetItemData(FName ItemName)
{
	if (auto ItemData = ItemInfos->ItemTable->FindRow<FItemData>(ItemName, "Found"))
	{
		return ItemData;
	}
	return nullptr;
}
