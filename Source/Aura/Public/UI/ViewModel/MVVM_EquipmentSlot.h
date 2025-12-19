// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "MVVM_EquipmentSlot.generated.h"

class UEquipmentComponent;
class UOverlayWidgetController;
class UInventoryComponent;
struct FItemData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSetIconDefault);

UCLASS()
class AURA_API UMVVM_EquipmentSlot : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
public:
	void Initialize(UEquipmentComponent* Equipment, uint8 Index);
	
	UFUNCTION(BlueprintCallable)
	void Reset();
	
	UFUNCTION()
	void ReInitializeSlotView();
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnSetIconDefault SetIconDefaultDelegate;
	
public:
	FName GetItemID() const {return ItemID;}
	void SetItemID(FName InID);
	
	EItemSubGroup GetItemSlotGroup() const {return ItemSlotGroup;}
	void SetItemSlotGroup(EItemSubGroup InSubGroup);
	
	UTexture2D* GetIcon() const {return Icon;}
	void SetIcon(UTexture2D* InIcon);
	
	FText GetDescription() const {return Description;}
	void SetDescription(FText InDesc);
	
	bool GetbEquipped() const {return bEquipped;}
	void SetbEquipped(bool Inbool);
	
protected:
	UPROPERTY()
	UInventoryComponent* InventoryComponent;
	
	UPROPERTY(BlueprintReadOnly)
	UEquipmentComponent* EquipmentComponent;
	
	UPROPERTY(BlueprintReadOnly)
	UOverlayWidgetController* OverlayWidgetController;
	
	// 필드 노티파이
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FName ItemID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	EItemSubGroup ItemSlotGroup;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	UTexture2D* Icon;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	bool bEquipped;
	
	// 아이템 설명
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FText Description = FText();
};
