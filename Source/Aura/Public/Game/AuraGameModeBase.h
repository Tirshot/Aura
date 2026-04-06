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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAllActorsInvincible, bool, bInvincible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSetActorInvincible, AActor*, TargetActor, bool, bInvincible);

UCLASS()
class AURA_API AAuraGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	AAuraGameModeBase();

protected:
	virtual void BeginPlay() override;
	
	// HUD가 생성되었을 때 호출될 함수 (델리게이트 바인딩)
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
public:
	UPROPERTY(BlueprintCallable)
	FOnSetActorInvincible OnSetActorInvincible;
	
	UPROPERTY(BlueprintCallable)
	FOnAllActorsInvincible OnAllActorsInvincible;
	
public:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual void RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot) override;
	
public:
	/* 
	 * 저장, 로드 관련
	 */
	static void DeleteSlot(const FString& SlotName, int32 SlotIndex);
	ULoadScreenSaveGame* RetrieveInGameSaveData();
	ULoadScreenSaveGame* RetrieveInGameSaveData(APlayerController* PC);
	
	void SaveInGameProgressData(ULoadScreenSaveGame* SaveObject);

	// 월드 저장 및 불러오기
	UFUNCTION()
	void SaveWorldState(UWorld* World, const FString& DestinationMapAssetName = FString(""));
	
	UFUNCTION()
	void SaveWorldStateAndTravel(UWorld* World, const FString& DestinationMapAssetName = FString(""));
	
	// 모든 클라이언트 대상으로 캐릭터 상태 저장
	UFUNCTION()
	void SaveAllCharacters();
	
	UFUNCTION()
	void SaveOneTimeUseActor(FGuid Guid, bool bUsed);
	
	UFUNCTION()
	void LoadWorldState(UWorld* World);
	
	void TravelToMap(UMVVM_LoadSlot* Slot);
	void TravelToMap(FString MapName);
	void ServerTravelToMap(FString MapName);
	
	UFUNCTION()
	void GameAutoSave();

	void RestartGameFromSaveData(ACharacter* DeadCharacter);
	void RestartGameFromSaveDataWithWorldContextObject(UObject* WorldContextObject);
	
	void PlayerRespawn(AAuraPlayerController* DeadPC);
	
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
	
	FAuraAbilityUpgradeInfo GetUpgradeRecursive(EUpgradeRarity Rarity, TMap<EUpgradeRarity, TArray<FAuraAbilityUpgradeInfo>>& Buckets);
	
	UFUNCTION()
	TArray<FAuraAbilityUpgradeInfo> GetRandomUpgradeInfosForActivatedAbility_Three(AAuraPlayerState* AuraPS);

public:
	/*
	 *	아이템 추가
	 */
	UFUNCTION(BlueprintCallable)
	bool GiveItemToCharacter(AAuraCharacter* Character, const FItemData& ItemData, int ItemCount = 1);
	void SpawnDropItemActor(AAuraCharacter* OwnedCharacter, const FItemData& DropItemData, FVector ItemSpawnLocation);
	
	UFUNCTION(BlueprintCallable)
	void SpawnDropItemToActorLocation(AActor* Actor, FName ItemID);
	
	UFUNCTION(BlueprintCallable)
	void SpawnDropItemToLocation(FVector Location, FName ItemID);
	
	// 아이템 드랍
	UFUNCTION()
	void DropItemOnMonsterDied(AAuraEnemy* DeadEnemy, AAuraCharacter* KilledBy);
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FString, TSoftObjectPtr<UWorld>> Maps;

	UFUNCTION(BlueprintCallable)
	FString GetMapNameFromMapAssetName(const FString& MapAssetName);

	UFUNCTION()
	void SetAllActorsInvincible(bool bInvincible);
	
	UFUNCTION()
	void SetActorInvincible(AActor* TargetActor, bool bInvincible);

public:
	// 몬스터에게 업그레이드 태그 부여
	UFUNCTION(BlueprintCallable)
	void AddAbilityUpgradeToEnemy(TSubclassOf<UGameplayEffect> AbilityUpgradeClass, AActor* ApplyActor);

	UFUNCTION(BlueprintCallable)
	void RemoveAbilityUpgradeFromEnemy(TSubclassOf<UGameplayEffect> AbilityUpgradeClass, AActor* ApplyActor);

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> Invincible3Sec;
};
