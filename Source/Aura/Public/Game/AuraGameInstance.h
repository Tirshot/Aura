// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "LoadScreenSaveGame.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Engine/GameInstance.h"
#include "AuraGameInstance.generated.h"

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
	
	// Handle Network Error 함수는 블루프린트로 오버라이드 가능
	
public:
	ULoadScreenSaveGame* GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const;
	class UCharacterClassInfo* GetCharacterClassInfo() {return CharacterClassInfo;}
	class UAbilityInfo* GetAbilityInfo() {return AbilityInfo;}
	class UAbilityUpgradeInfo* GetAbilityUpgradeInfo() {return AbilityUpgradeInfo;}
	
public:
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);
	
public:
	// 저장 슬롯
	UPROPERTY()
	FName PlayerStartTag = FName();
	
	UPROPERTY(BlueprintReadOnly)
	FString LoadSlotName = FString();

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
	
	UPROPERTY(EditDefaultsOnly, Category = "Revive Effect")
	TSubclassOf<UGameplayEffect> ReviveEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "LevelUp Effect")
	TSubclassOf<UGameplayEffect> FullHPMPEffect;
	
	FTimerHandle LoadMapTimer;
};
