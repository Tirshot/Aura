// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AuraGameStateBase.generated.h"

class UGameplayEffect;
class AAuraEnemy;
class AAuraBossMonster;
class AAuraPlayerController;
class AAuraDropItem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMonsterCountChanged, int32, MonsterCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossMonsterCountChanged, int32, BossCount);

UCLASS()
class AURA_API AAuraGameStateBase : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnBossMonsterCountChanged OnBossMonsterCountChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnMonsterCountChanged OnMonsterCountChanged;

public:
	UFUNCTION()
	void AddPlayerToArray(AAuraPlayerController* AuraPC);
	
	UFUNCTION()
	void AddMonsterToArray(AAuraEnemy* Enemy);
	void RemoveMonsterFromArray(AAuraEnemy* Enemy);
	
	UFUNCTION(BlueprintCallable)
	int32 GetBossCharacterArrayLength() { return BossCharacters.Num(); }
	
	UFUNCTION(BlueprintCallable)
	int32 GetMonsterCharacterLength() { return EnemyCharacters.Num(); }
	
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
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_BossCharactersSpawned();
	
	UFUNCTION(NetMulticast, Reliable)
	void MultiCast_OnBossMonsterDead(AActor* DeadActor);
	
	
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
	
};
