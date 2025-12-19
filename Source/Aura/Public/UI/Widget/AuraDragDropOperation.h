// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Blueprint/DragDropOperation.h"
#include "Player/InventoryComponent.h"
#include "AuraDragDropOperation.generated.h"


UCLASS()
class AURA_API UAuraDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()
	
protected:
	void DragCancelled_Implementation(const FPointerEvent& PointerEvent) override;

public:
	UPROPERTY(BlueprintReadWrite)
	FName ItemID;
	
	UPROPERTY(BlueprintReadWrite)
	int ItemCount;
	
	UPROPERTY(BlueprintReadWrite)
	int32 OriginalIndex;
	
	UPROPERTY(BlueprintReadWrite)
	FIntPoint ItemSize;
	
	UPROPERTY(BlueprintReadWrite)
	FItemData ItemData;
	
	UPROPERTY(BlueprintReadWrite)
	UInventoryComponent* Inventory;
	
	UPROPERTY(BlueprintReadWrite)
	bool bEquip = false;
};
