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
		auto ItemTable = AuraGI->GetItemInfos()->ItemTable;
	
		// 해당 위치부터 아이템 크기에 따라 배치 가능 판단
		if (IsItemStackable(ItemData))
		{
			// 중첩 가능
			FIntPoint Place;
			if (CanPlaceItem(ItemData, Place))
			{
				int StartX = Place.X;
				int StartY = Place.Y;
		
				for (int Y = StartY; Y < StartY + Height; Y++)
				{
					for (int X = StartX; X < StartX + Width; X++)
					{
						int NewIndex = InventoryWidth * Y + X;

						// 인벤토리 정보 채우기
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
							else if (AddCount >= 0)
								Slot.ItemCount = AddCount;
							
							OnItemGet.Broadcast(NewIndex);
						}
					}
				}
				OnRep_Slots();
				return true;
			}
			else
			{
				return false;
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
					int StartX = Place.X;
					int StartY = Place.Y;
		
					for (int Y = StartY; Y < StartY + Height; Y++)
					{
						for (int X = StartX; X < StartX + Width; X++)
						{
							int NewIndex = InventoryWidth * Y + X;

							// 인벤토리 정보 채우기
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
								Slot.ItemCount = 1;
								OnItemGet.Broadcast(NewIndex);
							}
						}
					}
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

bool UInventoryComponent::AddItemToIndex_Internal(const FItemData& ItemData, int index, int AddCount)
{
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

	// - 장비라면 빈 공간에 추가하기
	// 아이템 추가
	for (int Y = StartY; Y < StartY + Height; Y++)
	{
		for (int X = StartX; X < StartX + Width; X++)
		{
			int NewIndex = InventoryWidth * Y + X;

			// 인벤토리 정보 채우기
			FInventorySlot& Slot = Slots[NewIndex];
			
			Slot.bIsOccupied = true;
			Slot.ItemHandle.RowName = ItemData.Name;
			Slot.StartPoint = FIntPoint(StartX, StartY);
			Slot.ItemSize = FIntPoint(Width, Height);
			Slot.ItemData = ItemData;

			// 첫 칸에만 카운트 측정
			if (X == StartX && Y == StartY)
			{
				if (IsItemStackable(ItemData))
				{
					Slot.ItemCount = AddCount;
				}
				else
				{
					Slot.ItemCount = 1;
				}
			}
		}
	}
	
	if (!IsItemStackable(ItemData))
	{
		if (AddCount - 1 > 0)
			AddItem_Internal(ItemData, AddCount - 1);
	}
	
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
	if (!Slots.IsValidIndex(OriginalIndex))
		return;
	
	// 이동 전 슬롯의 데이터 가져오기
	const FInventorySlot& OriginalSlot = Slots[OriginalIndex];

	PrevSlots = Slots;
	
	FItemData MoveItem;
	MoveItem.Name = OriginalSlot.ItemHandle.RowName;
	MoveItem.Size = OriginalSlot.ItemSize;
	int AddCount = OriginalSlot.ItemCount;

	// 기존 슬롯 제거
	ClearItemSpace_Internal(OriginalSlot.StartPoint, OriginalSlot.ItemSize);

	// 슬롯에 아이템 추가
	if (AddItemToIndex_Internal(MoveItem, TargetIndex, AddCount))
	{
		// 이동 성공
		OnRep_Slots();
	}
	else if (AddItemToIndex_Internal(MoveItem, OriginalIndex, AddCount))
	{
		// 이동 실패, 원래 위치로 되돌림
		OnRep_Slots();
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
	
	OnItemRemoved.Broadcast(RemovedItem);
	OnRep_Slots();
}

void UInventoryComponent::Server_RemoveItemToWorld_Implementation(int OriginalIndex)
{
	if (!Slots.IsValidIndex(OriginalIndex))
		return;
	
	// 이동 전 슬롯의 데이터 가져오기
	const FInventorySlot& OriginalSlot = Slots[OriginalIndex];
	const FName OriginalItemName = OriginalSlot.ItemHandle.RowName;

	PrevSlots = Slots;
	
	// 기존 슬롯 제거
	ClearItemSpace_Internal(OriginalSlot.StartPoint, OriginalSlot.ItemSize);
	
	OnRep_Slots();
	
	// 월드에 아이템 액터 스폰
	if (auto AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
	{
		AActor* AvatarActor = GetOwner();
		FItemData ItemData;
		if (auto FoundData = GetItemData(OriginalItemName))
		{
			if (auto AuraCharacter = Cast<AAuraCharacter>(AvatarActor))
				AuraGM->SpawnDropItemActor(AuraCharacter, *FoundData, AvatarActor->GetActorLocation());
		}
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

const FInventorySlot* UInventoryComponent::FindItemSlot(FName ItemID, bool& bFound)
{
	for (const auto& Slot : Slots)
	{
		if (Slot.ItemHandle.RowName == ItemID)
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
	// UI 갱신
	for (int Index = 0; Index < Slots.Num(); Index++)
	{
		if (PrevSlots[Index] != Slots[Index])
			OnInventorySlotChanged.Broadcast(Index);
	}

	PrevSlots = Slots;
}

