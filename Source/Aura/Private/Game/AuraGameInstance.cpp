// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameInstance.h"
#include "Engine/Engine.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Game/LoadScreenSaveGame.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "UI/HUD/LoadScreenHUD.h"
#include "UI/ViewModel/MVVM_LoadScreen.h"

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
	
	// 네트워크 접속 실패 시 바인딩
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UAuraGameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UAuraGameInstance::HandleTravelFailure);
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

ULoadScreenSaveGame* UAuraGameInstance::GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const
{
	USaveGame* SaveGameObject = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		// 데이터가 있으면 불러오기
		SaveGameObject = UGameplayStatics::LoadGameFromSlot(SlotName, SlotIndex);
	}
	else
	{
		// 데이터가 없으면 생성
		SaveGameObject = UGameplayStatics::CreateSaveGameObject(LoadScreenSaveGameClass);
	}
	
	// 커스텀 세이브로 캐스팅 후 리턴
	ULoadScreenSaveGame* LoadScreenSaveGame = Cast<ULoadScreenSaveGame>(SaveGameObject);
	return LoadScreenSaveGame;
}

void UAuraGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (!World)
		return;
	
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ALoadScreenHUD* LoadHUD = PC->GetHUD<ALoadScreenHUD>())
		{
			LoadHUD->LoadScreenViewModel->NetworkErrorReceived.Broadcast(ErrorString);
		}
	}
}

void UAuraGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (!World)
		return;
	
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ALoadScreenHUD* LoadHUD = PC->GetHUD<ALoadScreenHUD>())
		{
			LoadHUD->LoadScreenViewModel->NetworkErrorReceived.Broadcast(ErrorString);
		}
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
	if (!ItemInfos)
	{
		ItemInfos = NewObject<UItemInfo>(this, ItemInfosClass);
	}
	
	return ItemInfos;
}

const FItemData* UAuraGameInstance::GetItemData(FName ItemName)
{
	if (ItemName.IsNone()) 
		return nullptr;
	
	if (auto ItemData = GetItemInfos()->GetItemDataByID(ItemName))
	{
		return ItemData;
	}
	return nullptr;
}
