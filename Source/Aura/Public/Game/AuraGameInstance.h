// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LoadScreenSaveGame.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "AbilitySystem/Data/SoundData.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "AuraGameInstance.generated.h"

class USettingsMenuWidgetController;
class USaveGame;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGameInstanceInitialized);

UCLASS()
class AURA_API UAuraGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;
	
	UPROPERTY(BlueprintAssignable)
	FOnGameInstanceInitialized OnInitialized;

	bool bInit = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bIsOnline = false;
	
	// Handle Network Error 함수는 블루프린트로 오버라이드 가능
	
public:
	ULoadScreenSaveGame* GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const;
	class UCharacterClassInfo* GetCharacterClassInfo() {return CharacterClassInfo;}
	class UAbilityInfo* GetAbilityInfo() {return AbilityInfo;}
	class UAbilityUpgradeInfo* GetAbilityUpgradeInfo() {return AbilityUpgradeInfo;}
	USettingsMenuWidgetController* GetSettingsMenuWidgetController();
	
public:
	void HostSession(FString MapName);
	void FindSession();
	
	UFUNCTION(BlueprintCallable)
	void DestroySession();
	
	void CancelFindSession();
	
protected:
	virtual void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	virtual void OnFindSessionsComplete(bool bWasSuccessful);
	virtual void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	virtual void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	
public:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	
public:
	// 저장 슬롯
	UPROPERTY()
	FName PlayerStartTag = FName();
	
	UPROPERTY(BlueprintReadOnly)
	FString LoadSlotName = FString();

	UPROPERTY(BlueprintReadOnly)
	FString LoadMapName = FString();
	
	UPROPERTY()
	int32 LoadSlotIndex = 0;
	
	// 기본 맵 이름
	UPROPERTY(EditDefaultsOnly)
	FString DefaultMapName;

	// Soft Object Ptr : 존재하기 전까지 메모리에 적재하지 않음
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DefaultMap;

	UPROPERTY(EditDefaultsOnly)
	FName DefaultPlayerStartTag;
	
	// 온라인
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	
public:
	// 디버그 옵션 Setter/Getter
	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	bool IsVisibleNextButton() const { return bVisibleNextButton; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	void SetVisibleNextButton(bool bVisible) { bVisibleNextButton = bVisible; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	bool IsVisibleLevelUpButton() const { return bVisibleLevelUpButton; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	void SetVisibleLevelUpButton(bool bVisible) { bVisibleLevelUpButton = bVisible; }
	
	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	bool IsAuraInvincible() const { return bAuraInvincible; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	void SetAuraInvincible(bool bInvincible) { bAuraInvincible = bInvincible; }
	
	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	bool IsAuraInfiniteMana() const { return bAuraInfiniteMana; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	void SetAuraInfiniteMana(bool bInfiniteMana) { bAuraInfiniteMana = bInfiniteMana; }

	void SetAllVariablesToDefault();

	// 아이템 정보
	UFUNCTION(BlueprintCallable, Category="Item")
	UItemInfo* GetItemInfos();

	const FItemData* GetItemData(FName ItemName);
	
protected:
	// 디버그 옵션 변수
	UPROPERTY()
	bool bVisibleNextButton = false;
	
	UPROPERTY()
	bool bVisibleLevelUpButton = false;
	
	UPROPERTY()
	bool bAuraInvincible = false;
	
	UPROPERTY()
	bool bAuraInfiniteMana = false;

public:
	// 사운드 믹스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundMix> SoundMix;
	
	// 사운드 데이터 에셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sound")
	TObjectPtr<USoundData> SoundData;
	
	// 설정 메뉴 위젯 컨트롤러
	UPROPERTY()
	TObjectPtr<USettingsMenuWidgetController> SettingsMenuWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USettingsMenuWidgetController> SettingsMenuWidgetControllerClass;
	
	// 아이템 정보
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item Data")
	TSubclassOf<UItemInfo> ItemInfosClass;

	UPROPERTY(BlueprintReadOnly, Category = "Item Data")
	TObjectPtr<UItemInfo> ItemInfos;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Message")
	UDataTable* MessageTable;
	
	FDataTableRowHandle DTRowHandle;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USaveGame> LoadScreenSaveGameClass;
	
	UPROPERTY(EditDefaultsOnly, Category="Character Class Default")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;

	UPROPERTY(EditDefaultsOnly, Category="Ability Info")
	TObjectPtr<UAbilityInfo> AbilityInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="Ability Upgrade Info")
	TObjectPtr<UAbilityUpgradeInfo> AbilityUpgradeInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="Ability Upgrade Info")
	TObjectPtr<class UEnemyAbilityUpgradeInfo> EnemyAbilityUpgradeInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="Item")
	TObjectPtr<class ULootTiers> LootTiers;
	
	UPROPERTY(EditDefaultsOnly, Category = "Item Drop")
	TSubclassOf<class AAuraDropItem> DropItemClass;
	
	// 사망 태그 부여 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> DeadTagEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> ReviveEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> FullHPMPEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> SecondaryAttributes;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> InputBlockEffectClass;
	
	FTimerHandle LoadMapTimer;
	FTimerHandle ReviveInvincibleTimer;
};
