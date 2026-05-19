// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/Data/MissionInfo.h"
#include "GameFramework/GameStateBase.h"
#include "AuraGameStateBase.generated.h"

struct FMissionData;
class UGameplayEffect;
class AAuraEnemy;
class AAuraBossMonster;
class AAuraPlayerController;
class AAuraDropItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerCountChanged, int32, PlayerCount, AAuraPlayerController*, JoinedPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterCountChanged, int32, MonsterCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossMonsterCountChanged, int32, BossCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionDataChangedSignature, const FMissionDataArray&, CurrentMissions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMissionFinishedSignature, FGameplayTag, MissionTag, bool, bSuccess);

UCLASS()
class AURA_API AAuraGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnPlayerCountChanged OnPlayerCountChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnBossMonsterCountChanged OnBossMonsterCountChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnMonsterCountChanged OnMonsterCountChanged;

	UPROPERTY(BlueprintAssignable)
	FOnMissionFinishedSignature OnMissionFinishedSignature;
	
public:
	UFUNCTION()
	void AddPlayerToArray(AAuraPlayerController* AuraPC);
	
	UFUNCTION()
	void RemovePlayerFromArray(AAuraPlayerController* AuraPC);
	
	UFUNCTION()
	void AddMonsterToArray(AAuraEnemy* Enemy);
	void RemoveMonsterFromArray(AAuraEnemy* Enemy);
	
	UFUNCTION(BlueprintCallable)
	int32 GetBossCharacterArrayLength() { return BossCharacters.Num(); }
	
	UFUNCTION(BlueprintCallable)
	int32 GetMonsterCharacterLength() { return EnemyCharacters.Num(); }
	
	const FMissionDataArray& GetCurrentMissions() { return CurrentMissions; }
	
public:
	// 드랍 아이템 관리
	void AddDroppedItem(AAuraDropItem* Item) { DroppedItems.Add(Item); }
	void RemoveDroppedItem(AAuraDropItem* Item) { DroppedItems.Remove(Item); }

public:
	const TArray<TSoftObjectPtr<AAuraEnemy>>& GetEnemyCharactersArray(){return EnemyCharacters;}
	const TArray<TSoftObjectPtr<AAuraBossMonster>>& GetBossCharactersArray(){return BossCharacters;}
	const TArray<TSoftObjectPtr<AAuraPlayerController>>& GetPlayersArray(){return Players;}
	const TArray<AAuraDropItem*>& GetDroppedItemsArray(){return DroppedItems;}
	
public:
	// 미션 관리
	void UpdateMissionData(FMissionData& MissionData);
	void RemoveMissionData(const FGameplayTag& MissionTag);
	void BroadcastMissionData();

	// 위젯 컨트롤러가 구독할 델리게이트
	UPROPERTY(BlueprintAssignable)
	FOnMissionDataChangedSignature OnMissionDataChanged;
	
public:
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_BossCharactersSpawned();
	
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnBossMonsterDead(AActor* DeadActor);
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_MissionFinished(const FGameplayTag& MissionTag, bool bIsSucceed);
	
private:
	// 액터 배열
	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	TArray<TSoftObjectPtr<AAuraEnemy>> EnemyCharacters;

	UPROPERTY()
	TArray<TSoftObjectPtr<AAuraBossMonster>> BossCharacters;
	
	UPROPERTY()
	TArray<TSoftObjectPtr<AAuraPlayerController>> Players;
	
	UPROPERTY()
	TArray<AAuraDropItem*> DroppedItems;
	
protected:
	UPROPERTY(Replicated)
	FMissionDataArray CurrentMissions;
};
