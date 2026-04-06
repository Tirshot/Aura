// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/InventoryComponent.h"

#include "AbilitySystem/Data/ItemInfo.h"
#include "Character/AuraCharacter.h"
#include "Game/AuraGameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Player/AuraPlayerController.h"
#include "Player/CharmComponent.h"

void FInventorySlot::PostReplicatedAdd(const FInventorySlotList& InArraySerializer)
{
	if (!Inventory)
		return;
	
	if (Inventory->OnInventorySlotChanged.IsBound())
		Inventory->OnInventorySlotChanged.Broadcast(SlotID);
}

void FInventorySlot::PostReplicatedChange(const FInventorySlotList& InArraySerializer)
{
	if (!Inventory)
		return;
	
	if (Inventory->OnInventorySlotChanged.IsBound())
		Inventory->OnInventorySlotChanged.Broadcast(SlotID);
}

void FInventorySlot::PreReplicatedRemove(const FInventorySlotList& InArraySerializer)
{
}

void FInventorySlotList::HandleUIUpdate(int32 Index)
{
	if (Inventory)
	{
		Inventory->OnInventorySlotChanged.Broadcast(Index);
	}
}

void FInventorySlotList::HandleUIUpdateToSlotList()
{
	if (Inventory && Inventory->OnInventorySlotChanged.IsBound())
	{
		for (int32 i = 0; i < Slots.Num(); ++i) 
		{
			Inventory->OnInventorySlotChanged.Broadcast(i);
		}
	}
}

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}


void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 서버 - 인벤토리 슬롯 배열 초기화
	if (GetOwnerRole() == ROLE_Authority)
	{
		if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner()))
		{
			if (AuraPS->bIsDataLoaded)
			{
				SlotList.Inventory = this;

				for (FInventorySlot& Slot : SlotList.Slots)
				{
					Slot.Inventory = this;
				}
				SlotList.MarkArrayDirty();
				return;
			}
			
			if (SlotList.Slots.IsEmpty())
			{
				for (int i = 0; i < InventorySize; i++)
				{
					FInventorySlot Slot;
					Slot.Inventory = this;
					Slot.SlotID = i;
					SlotList.Slots.Add(Slot);
				}
				SlotList.Inventory = this;
				SlotList.MarkArrayDirty();
		
				AssignDataTableToSlot();
			}
		}
	}
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 배열 복제
	DOREPLIFETIME(UInventoryComponent, SlotList);
	DOREPLIFETIME(UInventoryComponent, bLoaded);
}


void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UInventoryComponent::InventorySlotItemChanged(int32 Index)
{
	OnInventorySlotChanged.Broadcast(Index);
}

void UInventoryComponent::InventorySlotListChanged()
{
	for (int i = 0; i < SlotList.Slots.Num() - 1; i++)
	{
		OnInventorySlotChanged.Broadcast(i);
	}
}

void UInventoryComponent::AssignDataTableToSlot()
{
	UItemInfo* ItemInfos = nullptr;

	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return;
	
	ItemInfos = AuraGI->GetItemInfos();
	if (!ItemInfos)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInfos Not FOUND!!!"));
		return;
	}
		
	for (int i = 0; i < InventorySize; i++)
	{
		FInventorySlot& Slot = SlotList.Slots[i];
		Slot.ItemHandle.DataTable = ItemInfos->ItemTable;
		SlotList.MarkItemDirty(Slot);
	}
}

bool UInventoryComponent::CheckItemSpaceIsClear(int StartX, int StartY, int Width, int Height)
{
	// (1) 아이템이 인벤토리 우측 끝을 넘어감
	if (StartX + Width > InventoryWidth)
		return false;

	// (2) 아이템이 인벤토리 하단 끝을 넘어감
	if (StartY + Height > InventoryHeight)
		return false;

	// (3) 점유 확인
	for (int Y = StartY; Y < StartY + Height; Y++)
	{
		for (int X = StartX; X < StartX + Width; X++)
		{
			int Index = InventoryWidth * Y + X;
			if (Index < 0 || Index >= SlotList.Slots.Num())
				return false;

			if (SlotList.Slots[Index].bIsOccupied == true)
				return false;
		}
	}

	// 모든 칸이 비어있음
	return true;
}

bool UInventoryComponent::CanPlaceItem(const FItemData& ItemData, FIntPoint& OutPoint)
{
	// 아이템 크기
	int Width = ItemData.Size.X;
	int Height = ItemData.Size.Y;

	for (int Y = 0; Y <= InventoryHeight - Height; Y++)
	{
		for (int X = 0; X <= InventoryWidth - Width; X++)
		{
			// 해당 위치부터 아이템 크기에 따라 배치 가능 판단
			if (CheckItemSpaceIsClear(X, Y, Width, Height))
			{
				// 배치 가능한 공간을 찾으면 true
				OutPoint.X = X;
				OutPoint.Y = Y;
				return true;
			}
		}
	}

	return false;
}

bool UInventoryComponent::CanPlaceItemToIndex(const FItemData& ItemData, int index)
{
	if (index < 0 || index >= InventorySize)
		return false;
	
	// 아이템 크기
	int Width = ItemData.Size.X;
	int Height = ItemData.Size.Y;

	// 인덱스를 8로 나눈 나머지가 X(열)임, 몫이 Y(행)임
	int X = index % InventoryWidth;
	int Y = index / InventoryWidth;

	// 해당 위치부터 아이템 크기에 따라 배치 가능 판단
	if (CheckItemSpaceIsClear(X, Y, Width, Height))
	{
		// 배치 가능한 공간을 찾으면 true
		return true;
	}

	return false;
}

bool UInventoryComponent::PlaceItemAt(const FItemData& ItemData, int AddCount, int TargetIndex, bool bIsItemMoved)
{
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return false;
	
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		auto ItemTable = AuraGI->GetItemInfos()->ItemTable;
		int Width = ItemData.Size.X;
		int Height = ItemData.Size.Y;
		
		int StartX = TargetIndex % InventoryWidth;
		int StartY = TargetIndex / InventoryWidth;
		int NewIndex = 0;
		
		for (int Y = StartY; Y < StartY + Height; Y++)
		{
			for (int X = StartX; X < StartX + Width; X++)
			{
				NewIndex = InventoryWidth * Y + X;
            
				if (!SlotList.Slots.IsValidIndex(NewIndex))
					return false;
				
				if (!CanPlaceItemToIndex(ItemData, NewIndex))
					return false;
				
				FInventorySlot& Slot = SlotList.Slots[NewIndex];
				
				if (ItemTable)
					Slot.ItemHandle.DataTable = ItemTable;
						
				Slot.bIsOccupied = true;
				Slot.ItemHandle.RowName = ItemData.Name;
				Slot.StartPoint = FIntPoint(StartX, StartY);
				Slot.ItemSize = FIntPoint(Width, Height);
				Slot.ItemData = ItemData;
				Slot.Inventory = this;
				
				// 첫 칸일때
				if (X == StartX && Y == StartY)
				{
					if (AddCount == 0)
						Slot.ItemCount++;
					else if (AddCount > 0)
						Slot.ItemCount = AddCount;
								
					Slot.ItemData.ItemCounts = Slot.ItemCount;
				}
				SlotList.Inventory = this;
				SlotList.HandleUIUpdate(NewIndex);
				SlotList.MarkItemDirty(Slot);
			}
		}
		OnItemGet.Broadcast(NewIndex, bIsItemMoved);
		return true;
	}
	return false;
}

bool UInventoryComponent::ClearItemAt(int TargetIndex)
{
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return false;
	
	if (!SlotList.Slots.IsValidIndex(TargetIndex))
		return false;
	
	if (!SlotList.Slots[TargetIndex].bIsOccupied)
		return false;
	
	// 드래그 중인 아이템의 시작 좌표를 찾아서 넘김
	FIntPoint TargetPoint = SlotList.Slots[TargetIndex].StartPoint;
	FIntPoint ItemSize = SlotList.Slots[TargetIndex].ItemData.Size;
	
	return ClearItemSpace_Internal(TargetPoint, ItemSize);
}

bool UInventoryComponent::AddItem_Internal(const FItemData& ItemData, int AddCount)
{
	SlotList.Inventory = this;
	
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return false;
	
	if (!ItemData.Name.IsValid())
		return false;
	
	// - 장비라면 빈 공간에 추가하기
	// 해당 위치부터 아이템 크기에 따라 배치 가능 판단
	if (IsItemStackable(ItemData))
	{
		// 중첩 가능
		// 인벤토리에 있음
		if (HasItem(ItemData))
		{
			// 슬롯 찾기
			bool bFound = false;
			if (FInventorySlot* FoundSlot = FindItemSlot(ItemData.Name, bFound))
			{
				if (AddCount == 0)
				{
					FoundSlot->ItemCount++;
				}
				else
				{
					FoundSlot->ItemCount += AddCount;
				}
				FoundSlot->ItemData.ItemCounts = FoundSlot->ItemCount;
				SlotList.HandleUIUpdate(FoundSlot->SlotID);
				SlotList.MarkItemDirty(*FoundSlot);
				return true;
			}
		}
		else
		{
			// 인벤토리에 없음
			FIntPoint Place;
			if (CanPlaceItem(ItemData, Place))
			{
				int TargetIndex = (Place.Y * InventoryWidth) + Place.X;
				if (!PlaceItemAt(ItemData, AddCount, TargetIndex))
					return false;
			}
		}
	}
	else
	{
		// 중첩 불가
		for (int i = 0; i < AddCount; i++)
		{
			FIntPoint Place = {0, 0};
			if (CanPlaceItem(ItemData, Place))
			{
				int TargetIndex = (Place.Y * InventoryWidth) + Place.X;
				if (!PlaceItemAt(ItemData, AddCount, TargetIndex))
					return false;
			}
			else
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

bool UInventoryComponent::AddItemToIndex_Internal(const FItemData& ItemData, int index)
{
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return false;
	
	// 아이템을 다른 슬롯으로 이동시킬때 호출되는 함수
	if (!ItemData.Name.IsValid())
		return false;
	
	// 아이템 크기
	int Width = ItemData.Size.X;
	int Height = ItemData.Size.Y;

	// 인덱스를 8로 나눈 나머지가 X(열)임, 몫이 Y(행)임
	int StartX = index % InventoryWidth;
	int StartY = index / InventoryWidth;

	// 해당 위치부터 아이템 크기에 따라 배치 가능 판단
	if (!CheckItemSpaceIsClear(StartX, StartY, Width, Height))
		return false;

	// 아이템 추가
	if (!PlaceItemAt(ItemData, 1, index))
		return false;
	
	return true;
}

FInventorySlot* UInventoryComponent::GetSlotByIndex(int index)
{
	if (index >= InventorySize)
		return nullptr;
	
	if (!SlotList.Slots.IsValidIndex(index))
		return nullptr;

	return &SlotList.Slots[index];
}

bool UInventoryComponent::IsItemStackable(const FItemData& ItemData) const
{
	return ItemData.bStackable;
}

bool UInventoryComponent::HasItem(const FItemData& ItemData) const
{
	for (const auto& Slot : SlotList.Slots)
	{
		if (Slot.ItemData == ItemData)
			return true;
	}
	return false;
}

void UInventoryComponent::SetInventorySlots(const TArray<FInventorySlot>& InSlots)
{
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return;
	
	SlotList.Slots.Empty();
	SlotList.Inventory = this;
	SlotList.MarkArrayDirty();
    
	for (const FInventorySlot& NewSlot : InSlots)
	{
		FInventorySlot& AddedSlot = SlotList.Slots.Add_GetRef(NewSlot);
		AddedSlot.Inventory = this;
		OnItemGet.Broadcast(NewSlot.SlotID, false);
		SlotList.MarkItemDirty(AddedSlot); 
		
		// if (AddedSlot.ItemData.)
		// LoadedCharm.Add();
	}
	
	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner()))
	{
		if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(AuraPS->GetPlayerController()))
		{
			if (!AuraPC->OnCharacterInit.IsAlreadyBound(this, &UInventoryComponent::OnCharacterInitialized))
				AuraPC->OnCharacterInit.AddDynamic(this, &UInventoryComponent::OnCharacterInitialized);
		}
	}
	SlotList.MarkArrayDirty();
}

void UInventoryComponent::ForceReplication()
{
	if (GetOwnerRole() != ROLE_Authority)
		return;
	
	SlotList.MarkArrayDirty();
	
	GetOwner()->ForceNetUpdate();
}

void UInventoryComponent::Server_MoveItem_Implementation(int OriginalIndex, int TargetIndex)
{
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return;
	
	if (!SlotList.Slots.IsValidIndex(OriginalIndex) || !SlotList.Slots.IsValidIndex(TargetIndex))
		return;
	
	// 제자리에 놓기
	if (OriginalIndex == TargetIndex)
		return;
	
	// 이동 전 슬롯의 데이터 가져오기
	const FInventorySlot& OriginalSlot = SlotList.Slots[OriginalIndex];

	// 아이템 데이터 백업
	FItemData MoveItem = OriginalSlot.ItemData;
	
	// 아이템 먼저 제거
	if (ClearItemAt(OriginalIndex))
	{
		if (CanPlaceItemToIndex(MoveItem, TargetIndex))
		{
			PlaceItemAt(MoveItem, MoveItem.ItemCounts, TargetIndex, true);
		}
		else
		{
			// 배치 실패시 원상 복구
			PlaceItemAt(MoveItem, MoveItem.ItemCounts, OriginalIndex, true);
		}
	}
}

bool UInventoryComponent::ClearItemSpace_Internal(const FIntPoint& StartPoint, const FIntPoint& ItemSize)
{
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return false;
	
	SlotList.Inventory = this;
	
	// 인덱스를 8로 나눈 나머지가 X(열)임, 몫이 Y(행)임
	int StartX = StartPoint.X;
	int StartY = StartPoint.Y;
	float Width = ItemSize.X;
	float Height = ItemSize.Y;

	FItemData RemovedItem = SlotList.Slots[InventoryWidth * StartY + StartX].ItemData;
	
	// int StartIndex = InventorySize * StartY + StartX;

	for (int Y = StartY; Y < StartY + Height; Y++)
	{
		for (int X = StartX; X < StartX + Width; X++)
		{
			int Index = InventoryWidth * Y + X;

			// 점유하던 슬롯 초기화
			SlotList.Slots[Index].ItemHandle.RowName = "";
			SlotList.Slots[Index].ItemSize = FIntPoint();
			SlotList.Slots[Index].ItemCount = 0;
			SlotList.Slots[Index].StartPoint = FIntPoint();
			SlotList.Slots[Index].bIsOccupied = false;
			SlotList.Slots[Index].ItemData = FItemData();
			SlotList.HandleUIUpdate(Index);
			SlotList.MarkItemDirty(SlotList.Slots[Index]);
		}
	}
	return true;
}

void UInventoryComponent::RemoveItemToWorld(int OriginalIndex)
{
	if (APawn* AvatarActor = Cast<AAuraPlayerState>(GetOwner())->GetPawn())
	{
		if (AAuraPlayerController* AuraPC = AvatarActor->GetController<AAuraPlayerController>())
		{
			const FInventorySlot& TargetSlot = SlotList.Slots[OriginalIndex];
			const FItemData ItemDataToDrop = TargetSlot.ItemData;
			
			AuraPC->Server_TryRemoveItem(OriginalIndex);
		}
	}
}

void UInventoryComponent::RemoveItemToEquip(int OriginIndex)
{
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return;
	
	// 장착한 아이템은 인벤토리에서 제거
	if (OriginIndex <= -1 ||  OriginIndex >= GetInventorySize())
		return;
	
	if (!SlotList.Slots.IsValidIndex(OriginIndex))
		return;
	
	// 이동 전 슬롯의 데이터 가져오기
	const FInventorySlot& OriginalSlot = SlotList.Slots[OriginIndex];
	
	// 기존 슬롯 제거
	ClearItemSpace_Internal(OriginalSlot.StartPoint, OriginalSlot.ItemSize);
}

FInventorySlot* UInventoryComponent::FindItemSlot(FName ItemID, bool& bFound)
{
	for (auto& Slot : SlotList.Slots)
	{
		if (Slot.ItemHandle.RowName == ItemID && Slot.ItemCount >= 1)
		{
			bFound = true;
			return &Slot;
		}
	}

	bFound = false;
	return nullptr;
}

int32 UInventoryComponent::GetInventorySize() const
{
	return InventorySize;
}

int32 UInventoryComponent::GetInventoryWidth() const
{
	return InventoryWidth;
}

int32 UInventoryComponent::GetInventoryHeight() const
{
	return InventoryHeight;
}

const TArray<FInventorySlot>& UInventoryComponent::GetSlots() const
{
	return SlotList.Slots;
}

bool UInventoryComponent::IsFirstSlotOfItem(const FInventorySlot& Slot)
{
	int32 X = Slot.StartPoint.X;
	int32 Y = Slot.StartPoint.Y;
	
	return Slot.SlotID == (InventoryWidth * Y) + X;
}

int UInventoryComponent::GetDragItemIndex() const
{
	return DragItemIndex;
}

FItemData UInventoryComponent::GetCurrentDragItemData() const
{
	return CurrentDragItemData;
}

const FItemData* UInventoryComponent::GetItemData(FName ItemName)
{
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		return AuraGI->GetItemData(ItemName);
	}
	return nullptr;
}

void UInventoryComponent::OnRep_SlotList()
{
	for (auto& Slot : SlotList.Slots)
	{
		Slot.Inventory = this;
	}
	SlotList.Inventory = this;
	SlotsReplicated.Broadcast();
	bLoaded = true;
}

void UInventoryComponent::OnCharacterInitialized(ACharacter* Character)
{
	if (Character)
	{
		// UI 반영
		for (int i = 0; i < SlotList.Slots.Num(); i++)
		{
			if (OnInventorySlotChanged.IsBound())
				OnInventorySlotChanged.Broadcast(i);
		}
		if (Character->Implements<UPlayerInterface>())
		{
			EquipmentComponent = IPlayerInterface::Execute_GetEquipmentComponent(Character);
			if (EquipmentComponent)
			{
				if (EquipmentComponent->OnEquipmentSlotChanged.IsBound())
				{
					for (int i = 0; i < EquipmentComponent->EquipmentSlots.Items.Num(); i++)
					{
						EquipmentComponent->OnEquipmentSlotChanged.Broadcast(i);
					}
				}
			}
			
			CharmComponent = IPlayerInterface::Execute_GetCharmComponent(Character);
			if (CharmComponent)
			{
				CharmComponent->ApplyCharmEffectFromSavedInventory();
			}
		}
	}
}
