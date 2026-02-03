// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_EquipmentSlot.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Player/EquipmentComponent.h"


void UMVVM_EquipmentSlot::Initialize(UEquipmentComponent* Equipment, uint8 Index)
{
	EquipmentComponent = Equipment;
	OverlayWidgetController = UAuraAbilitySystemLibrary::GetOverlayWidgetController(this);
	
	// 열거형의 최대치(None)을 넘으면 리턴
	if (Index > static_cast<int8>(EItemSubGroup::None))
		return;
	
	EItemSubGroup ItemSubGroup = static_cast<EItemSubGroup>(Index);
	SetItemSlotGroup(ItemSubGroup);
}

void UMVVM_EquipmentSlot::Reset()
{
	EquippedItemData = FItemData();
	SetItemID("");
	SetIcon(nullptr);
	SetDescription(FText());
	SetbEquipped(false);
}

void UMVVM_EquipmentSlot::ReInitializeSlotView(const FItemData& ItemData)
{
	EquippedItemData = ItemData;
	SetItemID(ItemData.Name);
	SetIcon(ItemData.Image.Get());
	SetDescription(ItemData.Description);
	SetbEquipped(true);
}

void UMVVM_EquipmentSlot::SetItemID(FName InID)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemID, InID);
}

void UMVVM_EquipmentSlot::SetItemSlotGroup(EItemSubGroup InSubGroup)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemSlotGroup, InSubGroup);
}

void UMVVM_EquipmentSlot::SetIcon(UTexture2D* InIcon)
{
	UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon);
}

void UMVVM_EquipmentSlot::SetDescription(FText InDesc)
{
	UE_MVVM_SET_PROPERTY_VALUE(Description, InDesc);
}

void UMVVM_EquipmentSlot::SetbEquipped(bool Inbool)
{
	UE_MVVM_SET_PROPERTY_VALUE(bEquipped, Inbool);
}
