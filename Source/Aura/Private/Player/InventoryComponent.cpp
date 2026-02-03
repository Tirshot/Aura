// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/InventoryComponent.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Character/AuraCharacter.h"
#include "Game/AuraGameInstance.h"
#include "Game/AuraGameModeBase.h"
#include "Net/UnrealNetwork.h"
#include "UI/ViewModel/MVVM_Inventory.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}


void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 인벤토리 슬롯 배열 초기화
	for (int i = 0; i < InventorySize; i++)
	{
		FInventorySlot Slot;
		Slots.Add(Slot);
	}

	// 인벤토리 슬롯의 아이템 핸들에 데이터 테이블 등록
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		if (AuraGI->bInit)
		{
			AssignDataTableToSlot();
		}
		else
		{
			// 아직 초기화 전이면 델리게이트 등록
			AuraGI->OnInitialized.AddDynamic(this, &UInventoryComponent::AssignDataTableToSlot);
		}
	}
	
	UEquipmentComponent* EquipmentComponent = nullptr;
	if (GetOwner()->Implements<UPlayerInterface>())
	{
		EquipmentComponent = IPlayerInterface::Execute_GetEquipmentComponent(GetOwner());
	}
	
	// 인벤토리의 각 슬롯 뷰 모델 생성 및 초기화
	UAuraAbilitySystemLibrary::GetInventoryMenuViewModel(this)->InitializeSlots(this, EquipmentComponent);
	
	// 추후에 Slots 배열을 세이브 데이터로 집어 넣어야함(불러오기 및 저장)
}

void UInventoryComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 배열 복제
	DOREPLIFETIME(UInventoryComponent, Slots);
}


void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UInventoryComponent::AssignDataTableToSlot()
{
	UItemInfo* ItemInfos = nullptr;

	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
	{
		UE_LOG(LogTemp, Warning, TEXT("AuraGI Not FOUND!!!"));
		return;
	}
	
	ItemInfos = AuraGI->GetItemInfos();
	if (!ItemInfos)
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemInfos Not FOUND!!!"));
		return;
	}
		
	for (int i = 0; i < InventorySize; i++)
	{
		FInventorySlot& Slot = Slots[i];
		Slot.ItemHandle.DataTable = ItemInfos->ItemTable;
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
			if (Index < 0 || Index >= Slots.Num())
				return false;

			if (Slots[Index].bIsOccupied == true)
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
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		auto ItemTable = AuraGI->GetItemInfos()->ItemTable;
		int Width = ItemData.Size.X;
		int Height = ItemData.Size.Y;
		
		int StartX = TargetIndex % InventoryWidth;
		int StartY = TargetIndex / InventoryWidth;
		
		for (int Y = StartY; Y < StartY + Height; Y++)
		{
			for (int X = StartX; X < StartX + Width; X++)
			{
				int NewIndex = InventoryWidth * Y + X;

				// 인벤토리 정보 채우기
				int CurrentIndex = (Y * InventoryWidth) + X;
            
				if (!Slots.IsValidIndex(CurrentIndex))
					return false;
				
				FInventorySlot& Slot = Slots[NewIndex];
				
				if (ItemTable)
					Slot.ItemHandle.DataTable = ItemTable;
						
				Slot.bIsOccupied = true;
				Slot.ItemHandle.RowName = ItemData.Name;
				Slot.StartPoint = FIntPoint(StartX, StartY);
				Slot.ItemSize = FIntPoint(Width, Height);
				Slot.ItemData = ItemData;

				// 첫 칸일때
				if (X == StartX && Y == StartY)
				{
					if (AddCount == 0)
						Slot.ItemCount++;
					else if (AddCount > 0)
						Slot.ItemCount = AddCount;
								
					Slot.ItemData.ItemCounts = Slot.ItemCount;
								
					OnItemGet.Broadcast(NewIndex, bIsItemMoved);
				}
			}
		}
		OnRep_Slots();
		return true;
	}
	return false;
}

void UInventoryComponent::ClearItemAt(int TargetIndex)
{
	if (!Slots.IsValidIndex(TargetIndex))
		return;
	
	FIntPoint TargetPoint = FIntPoint(TargetIndex % InventoryWidth, TargetIndex / InventoryWidth);
	FIntPoint ItemSize = Slots[TargetIndex].ItemData.Size;
	
	ClearItemSpace_Internal(TargetPoint, ItemSize);
}

bool UInventoryComponent::AddItem_Internal(const FItemData& ItemData, int AddCount)
{
	if (!ItemData.Name.IsValid())
		return false;

	PrevSlots = Slots;
	
	// 아이템 크기
	int Width = ItemData.Size.X;
	int Height = ItemData.Size.Y;

	// - 장비라면 빈 공간에 추가하기
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
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
					
					OnRep_Slots();
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
				FIntPoint Place;
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
			OnRep_Slots();
			return true;
		}
	}
	return false;
}

bool UInventoryComponent::AddItemToIndex_Internal(const FItemData& ItemData, int index)
{
	// 아이템을 다른 슬롯으로 이동시킬때 호출되는 함수
	if (!ItemData.Name.IsValid())
		return false;
	
	PrevSlots = Slots;
	
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
	
	OnRep_Slots();
	return true;
}

FInventorySlot* UInventoryComponent::GetSlotByIndex(int index)
{
	if (index >= InventorySize)
		return nullptr;

	return &Slots[index];
}

bool UInventoryComponent::IsItemStackable(const FItemData& ItemData) const
{
	return ItemData.bStackable;
}

bool UInventoryComponent::HasItem(const FItemData& ItemData) const
{
	for (const auto& Slot : Slots)
	{
		if (Slot.ItemData == ItemData)
			return true;
	}
	return false;
}

void UInventoryComponent::SetInventorySlots(const TArray<FInventorySlot>& InSlots)
{
	Slots = InSlots;
	
	UEquipmentComponent* EquipmentComponent = nullptr;
	if (GetOwner()->Implements<UPlayerInterface>())
	{
		EquipmentComponent = IPlayerInterface::Execute_GetEquipmentComponent(GetOwner());
	}
	UAuraAbilitySystemLibrary::GetInventoryMenuViewModel(this)->InitializeSlots(this, EquipmentComponent);
}

void UInventoryComponent::Server_MoveItem_Implementation(int OriginalIndex, int TargetIndex)
{
	if (!Slots.IsValidIndex(OriginalIndex) || !Slots.IsValidIndex(TargetIndex))
		return;
	
	// 제자리에 놓기
	if (OriginalIndex == TargetIndex)
		return;
	
	// 이동 전 슬롯의 데이터 가져오기
	const FInventorySlot& OriginalSlot = Slots[OriginalIndex];

	PrevSlots = Slots;
	
	// 아이템 데이터 백업
	FItemData MoveItem = OriginalSlot.ItemData;
	
	// 아이템 먼저 제거
	ClearItemAt(OriginalIndex);
	
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

void UInventoryComponent::ClearItemSpace_Internal(const FIntPoint& StartPoint, const FIntPoint& ItemSize)
{
	PrevSlots = Slots;
	
	// 인덱스를 8로 나눈 나머지가 X(열)임, 몫이 Y(행)임
	int StartX = StartPoint.X;
	int StartY = StartPoint.Y;
	float Width = ItemSize.X;
	float Height = ItemSize.Y;

	FItemData RemovedItem = Slots[InventoryWidth * StartY + StartX].ItemData;
	
	// int StartIndex = InventorySize * StartY + StartX;

	for (int Y = StartY; Y < StartY + Height; Y++)
	{
		for (int X = StartX; X < StartX + Width; X++)
		{
			int Index = InventoryWidth * Y + X;

			// 점유하던 슬롯 초기화
			Slots[Index].ItemHandle.RowName = "";
			Slots[Index].ItemSize = FIntPoint();
			Slots[Index].ItemCount = 0;
			Slots[Index].StartPoint = FIntPoint();
			Slots[Index].bIsOccupied = false;
			Slots[Index].ItemData = FItemData();
			
			if (auto SlotViewModel = UAuraAbilitySystemLibrary::GetInventoryMenuViewModel(this)->GetSlotViewModel(Index))
			{
				// 슬롯의 뷰 모델도 초기화
				SlotViewModel->LoadInitialData();
			}
		}
	}
	
	OnRep_Slots();
}

void UInventoryComponent::Server_RemoveItemToWorld_Implementation(int OriginalIndex)
{
	if (!Slots.IsValidIndex(OriginalIndex))
		return;
	
	// 이동 전 슬롯의 데이터 가져오기
	const FInventorySlot& OriginalSlot = Slots[OriginalIndex];
	const FName OriginalItemName = OriginalSlot.ItemHandle.RowName;
	
	FItemData ItemData = OriginalSlot.ItemData;

	PrevSlots = Slots;
	
	// 기존 슬롯 제거
	ClearItemSpace_Internal(OriginalSlot.StartPoint, OriginalSlot.ItemSize);
	
	OnItemRemoved.Broadcast(ItemData);
	OnRep_Slots();
	
	// 월드에 아이템 액터 스폰
	if (auto AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
	{
		AActor* AvatarActor = GetOwner();
		if (auto AuraCharacter = Cast<AAuraCharacter>(AvatarActor))
			AuraGM->SpawnDropItemActor(AuraCharacter, ItemData, AvatarActor->GetActorLocation());
	}
}

void UInventoryComponent::Server_RemoveItemToEquip_Implementation(int OriginIndex)
{
	// 장착한 아이템은 인벤토리에서 제거
	if (OriginIndex <= -1 ||  OriginIndex >= GetInventorySize())
		return;
	
	if (!Slots.IsValidIndex(OriginIndex))
		return;
	
	// 이동 전 슬롯의 데이터 가져오기
	const FInventorySlot& OriginalSlot = Slots[OriginIndex];

	PrevSlots = Slots;
	
	// 기존 슬롯 제거
	ClearItemSpace_Internal(OriginalSlot.StartPoint, OriginalSlot.ItemSize);
	
	OnRep_Slots();
}

FInventorySlot* UInventoryComponent::FindItemSlot(FName ItemID, bool& bFound)
{
	for (auto& Slot : Slots)
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
	return Slots;
}

int UInventoryComponent::GetDragItemIndex() const
{
	return DragItemIndex;
}

FItemData UInventoryComponent::GetCurrentDragItemData() const
{
	return CurrentDragItemData;
}

FItemData* UInventoryComponent::GetItemData(FName ItemName)
{
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		auto ItemTable = AuraGI->GetItemInfos()->ItemTable;
		if (auto FoundData = ItemTable->FindRow<FItemData>(ItemName, "Found Item"))
		{
			return FoundData;
		}
	}
	return nullptr;
}

void UInventoryComponent::OnRep_Slots()
{
	if (PrevSlots.Num() <= 0)
		PrevSlots = Slots;
	
	if (Slots.Num() > 0)
	{
		// UI 갱신
		for (int Index = 0; Index < Slots.Num(); Index++)
		{
			if (PrevSlots[Index] != Slots[Index])
				OnInventorySlotChanged.Broadcast(Index);
		}
	}
	PrevSlots = Slots;
}

