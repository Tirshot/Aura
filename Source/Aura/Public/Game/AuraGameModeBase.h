// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AuraGameModeBase.generated.h"

class AAuraPlayerController;
struct FGameplayTagContainer;
class AAuraPlayerState;
class UAbilityUpgradeInfo;
class ULootTiers;
class ULoadScreenSaveGame;
class UCharacterClassInfo;
class UAbilityInfo;
class UMVVM_LoadSlot;
class USaveGame;

UCLASS()
class AURA_API AAuraGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	
	// HUD가 생성되었을 때 호출될 함수 (델리게이트 바인딩)
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
public:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

public:
	/* 
	 * 저장, 로드 관련
	 */
	void SaveSlotData(UMVVM_LoadSlot* LoadSlot, int32 SlotIndex);
	static void DeleteSlot(const FString& SlotName, int32 SlotIndex);
	ULoadScreenSaveGame* RetrieveInGameSaveData();
	void SaveInGameProgressData(ULoadScreenSaveGame* SaveObject);

	// 월드 저장 및 불러오기
	void SaveWorldState(UWorld* World, const FString& DestinationMapAssetName = FString(""));
	void LoadWorldState(UWorld* World);
	
	void TravelToMap(UMVVM_LoadSlot* Slot);

	ULoadScreenSaveGame* GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USaveGame> LoadScreenSaveGameClass;

	void PlayerDied(ACharacter* DeadCharacter, float RemainingTime);
	void RestartGameFromSaveData(ACharacter* DeadCharacter);
	void RestartGameFromSaveDataWithWorldContextObject(UObject* WorldContextObject);

public:
	/*
	 * 로그라이크
	 */
	UFUNCTION()
	void HandleInitializeCards(APlayerController* PC);
	
	UFUNCTION()
	void HandleRandomUpgradeTagsGenerated(AAuraPlayerState* AuraPS, TArray<FGameplayTag>& RandomUpgradeTags);

	UFUNCTION()
	void HandlePlayerStateInitialized(AAuraPlayerState* InitializedPlayerState);
	
	UFUNCTION()
	TArray<FAuraAbilityUpgradeInfo> GetRandomUpgradeInfosForActivatedAbility_Three(AAuraPlayerState* AuraPS);
	
public:
	UPROPERTY(EditDefaultsOnly, Category="Character Class Default")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;

	UPROPERTY(EditDefaultsOnly, Category="Ability Info")
	TObjectPtr<UAbilityInfo> AbilityInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="Ability Upgrade Info")
	TObjectPtr<UAbilityUpgradeInfo> AbilityUpgradeInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="Item")
	TObjectPtr<ULootTiers> LootTiers;
	
	// 기본 맵 이름
	UPROPERTY(EditDefaultsOnly)
	FString DefaultMapName;

	// Soft Object Ptr : 존재하기 전까지 메모리에 적재하지 않음
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DefaultMap;

	UPROPERTY(EditDefaultsOnly)
	FName DefaultPlayerStartTag;

	UPROPERTY(EditDefaultsOnly)
	TMap<FString, TSoftObjectPtr<UWorld>> Maps;

	FString GetMapNameFromMapAssetName(const FString& MapAssetName);
};
