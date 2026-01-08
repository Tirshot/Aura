// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/EnemyAbilityUpgradeInfo.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Character/AuraCharacterBase.h"
#include "GameFramework/GameModeBase.h"
#include "AuraGameModeBase.generated.h"

class AAuraCharacter;
struct FGameplayTag;
struct FAuraAbilityUpgradeInfo;
class UGameplayEffect;
class AAuraBossMonster;
class AAuraEnemy;
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
class AAuraDropItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterCountChanged, int32, MonsterCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossMonsterCountChanged, int32, BossCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllActorsInvincible, bool, bInvincible);

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
	void TravelToMap(FString MapName);

	ULoadScreenSaveGame* GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const;
	
	void GameAutoSave();
	
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
	/*
	 *	아이템 추가
	 */
	UFUNCTION(BlueprintCallable)
	bool GiveItemToCharacter(AAuraCharacter* Character, FName ItemID, int ItemCount = 1);
	void SpawnDropItemActor(AAuraCharacter* OwnedCharacter, FItemData DropItemData, FVector ItemSpawnLocation);
	
	UFUNCTION(BlueprintCallable)
	void SpawnDropItemToActorLocation(AAuraCharacter* Character, FName ItemID);

	// 아이템 드랍
	UFUNCTION()
	void DropItemOnMonsterDied(AAuraEnemy* DeadEnemy, AAuraCharacter* KilledBy);
	
public:
	UPROPERTY(EditDefaultsOnly, Category="Character Class Default")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;

	UPROPERTY(EditDefaultsOnly, Category="Ability Info")
	TObjectPtr<UAbilityInfo> AbilityInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="Ability Upgrade Info")
	TObjectPtr<UAbilityUpgradeInfo> AbilityUpgradeInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="Ability Upgrade Info")
	TObjectPtr<UEnemyAbilityUpgradeInfo> EnemyAbilityUpgradeInfo;
	
	UPROPERTY(EditDefaultsOnly, Category="Item")
	TObjectPtr<ULootTiers> LootTiers;
	
	UPROPERTY(EditDefaultsOnly, Category = "Item Drop")
	TSubclassOf<AAuraDropItem> DropItemClass;
	
	// 기본 맵 이름
	UPROPERTY(EditDefaultsOnly)
	FString DefaultMapName;

	// Soft Object Ptr : 존재하기 전까지 메모리에 적재하지 않음
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DefaultMap;

	UPROPERTY(EditDefaultsOnly)
	FName DefaultPlayerStartTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FString, TSoftObjectPtr<UWorld>> Maps;

	UFUNCTION(BlueprintCallable)
	FString GetMapNameFromMapAssetName(const FString& MapAssetName);

public:
	UFUNCTION()
	void OnBossMonsterDead(AActor* DeadActor);
	
	UFUNCTION()
	void AddMonsterToArray(AAuraEnemy* Enemy);
	void RemoveMonsterFromArray(AAuraEnemy* Enemy);

	UFUNCTION(BlueprintCallable)
	int32 GetBossCharacterArrayLength() { return BossCharacters.Num(); }
	
	UFUNCTION(BlueprintCallable)
	int32 GetMonsterCharacterLength() { return EnemyCharacters.Num(); }

	UFUNCTION()
	void SetAllActorsInvincible(bool bInvincible);

	UPROPERTY(BlueprintCallable)
	FOnAllActorsInvincible OnAllActorsInvincible;
	
	UPROPERTY(BlueprintAssignable)
	FOnBossMonsterCountChanged OnBossMonsterCountChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnMonsterCountChanged OnMonsterCountChanged;

	TArray<TSoftObjectPtr<AAuraBossMonster>> GetBossCharactersArray(){return BossCharacters;}

public:
	// 몬스터에게 업그레이드 태그 부여
	UFUNCTION(BlueprintCallable)
	void AddAbilityUpgradeToEnemy(TSubclassOf<UGameplayEffect> AbilityUpgradeClass, AActor* ApplyActor);

	UFUNCTION(BlueprintCallable)
	void RemoveAbilityUpgradeFromEnemy(TSubclassOf<UGameplayEffect> AbilityUpgradeClass, AActor* ApplyActor);


private:
	// 액터 배열
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<AAuraEnemy>> EnemyCharacters;

	UPROPERTY()
	TArray<TSoftObjectPtr<AAuraBossMonster>> BossCharacters;
};
