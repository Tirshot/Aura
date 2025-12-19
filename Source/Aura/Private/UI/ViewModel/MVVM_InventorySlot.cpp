// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_InventorySlot.h"

#include "Game/AuraGameInstance.h"

void UMVVM_InventorySlot::Initialize(UInventoryComponent* Component, int32 Index)
{
	// 참조 저장
	Inventory = Component;
	SlotIndex = Index;
        
	// 초기 데이터 로드
	LoadInitialData();
}

void UMVVM_InventorySlot::LoadInitialData()
{
	const FInventorySlot* Slot = Inventory->GetSlotByIndex(SlotIndex);
	if (!Slot)
		return;
	
	SetItemID(Slot->ItemHandle.RowName);
	SetItemCount(Slot->ItemCount);
	SetItemSize(Slot->ItemSize);
	SetStartPoint(Slot->StartPoint);
	SetbIsOccupied(Slot->bIsOccupied);
	
	if (FItemData* FoundData = Slot->ItemHandle.GetRow<FItemData>("GetItemData"))
	{
		ItemData = *FoundData;
		SetItemDescription(ItemData.Description);
	}

	// 이미지는 상위 뷰 모델에서 변경
}

void UMVVM_InventorySlot::ClearSlot()
{
	
}

void UMVVM_InventorySlot::SetItemID(const FName InID)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemID, InID);
}

void UMVVM_InventorySlot::SetItemCount(const int32 InCount)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemCount, InCount);
}

void UMVVM_InventorySlot::SetItemSize(const FIntPoint InSize)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemSize, InSize);
}

void UMVVM_InventorySlot::SetStartPoint(const FIntPoint InStartPoint)
{
	UE_MVVM_SET_PROPERTY_VALUE(StartPoint, InStartPoint);
}

void UMVVM_InventorySlot::SetbIsOccupied(const bool InBoolean)
{
	UE_MVVM_SET_PROPERTY_VALUE(bIsOccupied, InBoolean);
}

void UMVVM_InventorySlot::SetItemImage(UTexture2D* InImage)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemImage, InImage);
}

void UMVVM_InventorySlot::SetItemImageWidth(float InWidth)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemImageWidth, InWidth);
}

void UMVVM_InventorySlot::SetItemImageHeight(float InHeight)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemImageHeight, InHeight);
}

void UMVVM_InventorySlot::SetItemDescription(FText InDescription)
{
	UE_MVVM_SET_PROPERTY_VALUE(ItemDescription, InDescription);
}
