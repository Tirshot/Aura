// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_Inventory.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Character/AuraCharacter.h"
#include "Player/AuraPlayerController.h"
#include "Player/EquipmentComponent.h"
#include "Player/InventoryComponent.h"
#include "UI/HUD/AuraHUD.h"
#include "UI/ViewModel/MVVM_EquipmentSlot.h"

void UMVVM_Inventory::BindDependencies()
{

}

UWorld* UMVVM_Inventory::GetWorld() const
{
	if (const UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	return nullptr;
}

void UMVVM_Inventory::InitializeSlots(UInventoryComponent* InventoryComponent, UEquipmentComponent* EquipmentComponent)
{
	 if (InventoryComponent)
	 	Inventory = InventoryComponent;
	 if (EquipmentComponent)
	 	Equipment = EquipmentComponent;
	
	if (!Inventory || !Equipment)
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetOuter()))
		{
			if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(AuraHUD->GetOwningPlayerController()))
			{
				if (APawn* PlayerPawn = PC->GetPawn()) 
				{
					if (AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(PlayerPawn))
					{
						if (!Inventory)
							Inventory = IPlayerInterface::Execute_GetInventoryComponent(AuraCharacter);
						
						if (!Equipment)
							Equipment = IPlayerInterface::Execute_GetEquipmentComponent(AuraCharacter);
					}
				}
			}
		}
	}
	if (!IsValid(Inventory) || !IsValid(Equipment))
		return;
	

	const int32 Size = Inventory->GetSlots().Num(); 
	SlotViewModels.Empty();
	SlotViewModels.SetNum(Size);

	// 모든 슬롯에 대해 각자의 뷰 모델 연결
	for (int i = 0; i < Size; i++)
	{
		UMVVM_InventorySlot* NewSlotViewModel = NewObject<UMVVM_InventorySlot>(this);

		// 뷰 모델에 인덱스와 인벤토리 컴포넌트 포인터 넘겨주기
		NewSlotViewModel->Initialize(Inventory, i);

		// 슬롯 뷰 모델들을 배열로 관리
		SlotViewModels[i] = NewSlotViewModel;
	}

	Inventory->OnInventorySlotChanged.RemoveDynamic(this, &UMVVM_Inventory::InventorySlotChanged);
	Inventory->OnInventorySlotChanged.AddDynamic(this, &UMVVM_Inventory::InventorySlotChanged);

	// 실제 데이터를 슬롯 뷰 모델들에게 전달
	for (int i = 0; i < Size; ++i)
	{
		InventorySlotChanged(i);
	}
	
	// 장착 슬롯 뷰모델 생성 후 연결
	if (EquipSlotViewModels.Num() <= 0)
	{
		for (int i = 0; i < 4; i++)
		{
			UMVVM_EquipmentSlot* NewSlotViewModel = NewObject<UMVVM_EquipmentSlot>(this);

			// 뷰 모델에 인덱스와 장착 컴포넌트 포인터 넘겨주기
			NewSlotViewModel->Initialize(EquipmentComponent, i);

			// 슬롯 뷰 모델들을 배열로 관리
			EquipSlotViewModels.Add(NewSlotViewModel);
		}
	}
	
	Equipment->OnEquipmentSlotChanged.RemoveDynamic(this, &UMVVM_Inventory::EquipmentSlotChanged);
	Equipment->OnEquipmentSlotChanged.AddDynamic(this, &UMVVM_Inventory::EquipmentSlotChanged);
}

void UMVVM_Inventory::InventorySlotChanged(int Index)
{
	const TArray<FInventorySlot>& InventorySlots = Inventory->GetSlots();

	if (!InventorySlots.IsValidIndex(Index))
		return;
	
	const FInventorySlot& SlotData = InventorySlots[Index];
	UMVVM_InventorySlot* SlotViewModel = SlotViewModels[Index];
	
	SlotViewModel->ItemData = SlotData.ItemData;
	
	SlotViewModel->SetItemID(SlotData.ItemHandle.RowName);
	SlotViewModel->SetItemCount(SlotData.ItemCount);
	SlotViewModel->SetItemSize(SlotData.ItemSize);
	SlotViewModel->SetStartPoint(SlotData.StartPoint);
	SlotViewModel->SetbIsOccupied(SlotData.bIsOccupied);
	
	if (SlotViewModel->GetItemCount() != 0)
	{
		if (SlotData.ItemHandle.IsNull())
			return;

		if (auto FoundRow = SlotData.ItemHandle.GetRow<FItemData>(TEXT("InventorySlotChanged")))
		{
			SlotViewModel->SetItemImage(FoundRow->Image.Get());

			float ImageWidth = 67 * SlotData.ItemSize.X;
			float ImageHeight = 67 * SlotData.ItemSize.Y;
			SlotViewModel->SetItemImageWidth(ImageWidth);
			SlotViewModel->SetItemImageHeight(ImageHeight);

			SlotViewModel->SetItemDescription(FoundRow->Description);
		}
	}
}

void UMVVM_Inventory::EquipmentSlotChanged(int Index)
{
	const auto& EquipmentSlotEntry = Equipment->GetSlots();

	if (!EquipmentSlotEntry.Items.IsValidIndex(Index))
		return;
	
	UMVVM_EquipmentSlot* SlotViewModel = EquipSlotViewModels[Index];
	if (SlotViewModel)
	{
		SlotViewModel->ReInitializeSlotView(EquipmentSlotEntry.Items[Index].ItemData);
	}
}

void UMVVM_Inventory::SetInventorySlotEmpty(const FIntPoint& StartPoint, const FIntPoint& ItemSize)
{
	// 인덱스를 8로 나눈 나머지가 X(열)임, 몫이 Y(행)임
	int StartX = StartPoint.X;
	int StartY = StartPoint.Y;
	float Width = ItemSize.X;
	float Height = ItemSize.Y;

	// int StartIndex = InventorySize * StartY + StartX;

	for (int Y = StartY; Y < StartY + Height; Y++)
	{
		for (int X = StartX; X < StartX + Width; X++)
		{
			int Index = Inventory->GetInventoryWidth() * Y + X;

			// 점유하던 슬롯 초기화
			SlotViewModels[Index]->SetItemID("");
			SlotViewModels[Index]->SetItemSize(FIntPoint());
			SlotViewModels[Index]->SetItemCount(0);
			SlotViewModels[Index]->SetStartPoint(FIntPoint());
			SlotViewModels[Index]->SetbIsOccupied(false);
		}
	}
}

void UMVVM_Inventory::SetSlotViewModels(const TArray<UMVVM_InventorySlot*>& InSlots)
{
	UE_MVVM_SET_PROPERTY_VALUE(SlotViewModels, InSlots);
}

void UMVVM_Inventory::SetDummyString(FString InStr)
{
	UE_MVVM_SET_PROPERTY_VALUE(DummyString, InStr);
}

UMVVM_EquipmentSlot* UMVVM_Inventory::GetEquipSlotViewModel(EItemSubGroup ItemSubGroup)
{
	int8 Index = static_cast<int8>(ItemSubGroup);
	int8 Maximum = static_cast<int8>(EItemSubGroup::None);
	if (Index < Maximum && Index >= 0)
	{
		return EquipSlotViewModels[Index];
	}
	return nullptr;
}

TArray<UMVVM_EquipmentSlot*> UMVVM_Inventory::GetAllEquipSlotViewModels()
{
	return EquipSlotViewModels;
}

UMVVM_InventorySlot* UMVVM_Inventory::GetSlotViewModel(int Index)
{
	if (Index >= 0 && Index < SlotViewModels.Num())
		return SlotViewModels[Index];
	
	return nullptr;
}
