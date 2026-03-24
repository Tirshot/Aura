// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_EquipmentSlot.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Game/AuraGameInstance.h"


void UMVVM_EquipmentSlot::Initialize(UEquipmentComponent* Equipment, uint8 Index)
{
	EquipmentComponent = Equipment;
	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(Equipment->GetOwner()))
	{
		OverlayWidgetController = UAuraAbilitySystemLibrary::GetOverlayWidgetController(AuraPS->GetPlayerController());
	}
	
	switch (Index)
	{
	case 0:
		SetItemSlotGroup(EItemSubGroup::Helmet);
		break;
		
	case 1:
		SetItemSlotGroup(EItemSubGroup::Armor);
		break;
		
	case 2:
		SetItemSlotGroup(EItemSubGroup::Boots);
		break;
		
	case 3:
		SetItemSlotGroup(EItemSubGroup::Weapon);
		break;
		
	default:
		break;
	}
	
	LoadInitialData();
}

void UMVVM_EquipmentSlot::LoadInitialData()
{
	const FEquipmentSlot* Slot = EquipmentComponent->GetSlotEntry(ItemSlotGroup);
	if (!Slot)
		return;
	
	SetItemID(Slot->ItemData.Name);
	SetIcon(Slot->ItemData.Image);
	SetDescription(Slot->ItemData.Description);
	SetbEquipped(true);
}

void UMVVM_EquipmentSlot::Reset()
{
	EquippedItemData = FItemData();
	SetItemID("");
	SetIcon(DefaultImage);
	SetDescription(FText());
	SetbEquipped(false);
}

void UMVVM_EquipmentSlot::ReInitializeSlotView(const FItemData& ItemData)
{
	FItemData CopiedItemData = ItemData;
	
	EquippedItemData = CopiedItemData;
	SetItemID(CopiedItemData.Name);
	SetDescription(CopiedItemData.Description);
	SetbEquipped(true);
	
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		if (const FItemData* FoundItemData = AuraGI->GetItemData(ItemData.Name))
		{
			if (auto* FoundIcon = FoundItemData->Image.Get())
			{
				SetIcon(FoundIcon);
				return;
			}
		}
	}
	SetIcon(DefaultImage);
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
