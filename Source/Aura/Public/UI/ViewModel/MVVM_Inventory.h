// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MVVM_EquipmentSlot.h"
#include "MVVM_InventorySlot.h"
#include "UI/Widget/ItemToolTipWidget.h"
#include "MVVM_Inventory.generated.h"

class UEquipmentComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemDataMissing, int, DataMissingSlotIndex);

UCLASS()
class AURA_API UMVVM_Inventory : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void BindDependencies();

public:
	FOnItemDataMissing DataMissingSlotIndex;
	
	UFUNCTION(BlueprintCallable)
	void InitializeSlots(class UInventoryComponent* InventoryComponent, UEquipmentComponent* EquipmentComponent);
	
	UFUNCTION(BlueprintCallable)
	void InventorySlotChanged(int Index);
	
	UFUNCTION(BlueprintCallable)
	void SetInventorySlotEmpty(const FIntPoint& StartPoint, const FIntPoint& ItemSize);

public:
	UMVVM_InventorySlot* GetSlotViewModel(int Index);
	
public:
	const TArray<UMVVM_InventorySlot*>& GetSlotViewModels() const {return SlotViewModels;}
	void SetSlotViewModels(const TArray<UMVVM_InventorySlot*>& InSlots);

	FString GetDummyString() const {return DummyString;}
	void SetDummyString(FString InStr);
	
	UFUNCTION(BlueprintCallable)
	UMVVM_EquipmentSlot* GetEquipSlotViewModel(EItemSubGroup ItemSubGroup);
	
	UFUNCTION(BlueprintCallable)
	TArray<UMVVM_EquipmentSlot*> GetAllEquipSlotViewModels();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	TArray<UMVVM_InventorySlot*> SlotViewModels;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<UMVVM_EquipmentSlot*> EquipSlotViewModels;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FString DummyString;
	
	UPROPERTY(BlueprintReadWrite)
	int32 DragItemInitialIndex;
	
	UPROPERTY(BlueprintReadOnly)
	class UInventoryComponent* Inventory;
	
public:
	UPROPERTY(BlueprintReadOnly)
	UItemToolTipWidget* ItemToolTipWidget;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UItemToolTipWidget> ItemToolTipWidgetClass;
	
};
