// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraPlayerState.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Components/ActorComponent.h"
#include "CharmComponent.generated.h"

struct FActiveGameplayEffectHandle;
class UInventoryComponent;

USTRUCT(BlueprintType)
struct FCharmActiveEffects
{
	GENERATED_BODY()

	// 이 아이템이 적용한 이펙트 핸들들
	UPROPERTY()
	TArray<FActiveGameplayEffectHandle> EffectHandles;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AURA_API UCharmComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCharmComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void TryToBindItemGetAndRemove();

public:	
	void ApplyCharmEffectFromSavedInventory();
	void RemoveCharmItemEffect(const FItemData& CharmItem);
	void ApplyCharmItemEffect(const FItemData& CharmItem);

public:
	UFUNCTION()
	void AddToCharmSlot(int SlotIndex, bool bIsItemMoved);
	
	UFUNCTION()
	void RemoveFromCharmSlot(const FItemData& ItemData);
	
	void ApplyItemStat(const FItemData& ItemData);
	
public:
	UFUNCTION()
	void OnPawnSet(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn);
		
protected:
	// 아이템의 스텟, 기타 게임플레이 이펙트도 동시에 관리
	UPROPERTY()
	TMap<FGuid, FCharmActiveEffects> AppliedCharms;
};
