// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Components/ActorComponent.h"
#include "CharmComponent.generated.h"

class UInventoryComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AURA_API UCharmComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCharmComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void RemoveCharmItemEffects();
	void ApplyCharmItemEffects();

public:
	UFUNCTION()
	void AddToCharmSlot(int SlotIndex);
	
	UFUNCTION()
	void RemoveFromCharmSlot(FItemData ItemData);
	
	void ApplyItemStat(const FItemData& ItemData);
		
protected:
	TArray<FItemData> CharmSlotArray;
};
