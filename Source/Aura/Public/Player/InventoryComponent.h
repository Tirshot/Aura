// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

USTRUCT(BlueprintType)
struct FInventorySlot
{
	GENERATED_BODY()

	// 각 인벤토리 칸
	// 실제 데이터를 가지는 구조체
	UPROPERTY(BlueprintReadOnly)
	FDataTableRowHandle ItemHandle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FItemData ItemData;
	
	// 아이템 갯수
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 ItemCount = 0;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint ItemSize = FIntPoint::ZeroValue;

	// 아이템 좌상단 시작 위치
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FIntPoint StartPoint = FIntPoint::ZeroValue;
	
	// 해당 칸이 점유되었는가
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsOccupied = false; 
	
	// 인덱스 공식
	// 슬롯 = 열 x 행
	// 인덱스 = 슬롯 가로크기(8) x (행-1) + (열-1)
	// idx = InventoryWidth * Y + X;

	bool operator==(const FInventorySlot& Other) const
	{
		return ItemHandle == Other.ItemHandle && ItemCount == Other.ItemCount && bIsOccupied == Other.bIsOccupied && ItemSize == Other.ItemSize && StartPoint == Other.StartPoint;
	}

	bool operator!=(const FInventorySlot& Other) const
	{
		return !(*this == Other);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotChanged, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemGet, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemRemoved, const FItemData&, RemovedItemData);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AURA_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UInventoryComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnInventorySlotChanged OnInventorySlotChanged;

	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnItemGet OnItemGet;
	
	UPROPERTY(BlueprintAssignable, Category="Inventory")
	FOnItemRemoved OnItemRemoved;
	
	UFUNCTION()
	void AssignDataTableToSlot();
	
public:
	// 해당 칸부터 아이템 크기만큼 배치 가능 여부 판단
	UFUNCTION(BlueprintCallable)
	bool CheckItemSpaceIsClear(int StartX, int StartY, int Width, int Height);
	
	// 아이템 습득 시 배치 가능 여부 판단
	UFUNCTION(BlueprintCallable)
	bool CanPlaceItem(const FItemData& ItemData, FIntPoint& OutPoint);

	// 아이템을 인덱스로 추가할 때 배치 가능 여부 판단
	UFUNCTION(BlueprintCallable)
	bool CanPlaceItemToIndex(const FItemData& ItemData, int index);

	FInventorySlot* GetSlotByIndex(int index);

	bool IsItemStackable(const FItemData& ItemData) const;
	
	bool HasItem(const FItemData& ItemData) const;

	// 불러오기 용도
	void SetInventorySlots(const TArray<FInventorySlot>& InSlots);

public:
	UFUNCTION()
	bool AddItem_Internal(const FItemData& ItemData, int AddCount = 1);

	UFUNCTION()
	bool AddItemToIndex_Internal(const FItemData& ItemData, int index);

	// 드래그를 위해 집어들었을 때 기존 데이터 정리
	UFUNCTION()
	void ClearItemSpace_Internal(const FIntPoint& StartPoint, const FIntPoint& ItemSize);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_MoveItem(int OriginalIndex, int TargetIndex);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RemoveItemToWorld(int OriginalIndex);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RemoveItemToEquip(int OriginIndex);
	
	// 아이템 슬롯 찾기
	FInventorySlot* FindItemSlot(FName ItemID, bool& bFound);

public:
	int32 GetInventorySize() const;
	int32 GetInventoryWidth() const;
	int32 GetInventoryHeight() const;
	
	UFUNCTION(BlueprintCallable)
	const TArray<FInventorySlot>& GetSlots() const;
	
	UFUNCTION(BlueprintCallable)
	int GetDragItemIndex() const;
	
	UFUNCTION(BlueprintCallable)
	void SetDragItemIndex(int DragIndex) {DragItemIndex = DragIndex;}
	
	FItemData GetCurrentDragItemData() const;
	FItemData* GetItemData(FName ItemName);
	
public:
	// 참 아이템 효과 적용
	
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	int32 InventorySize = 40;
	
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	int32 InventoryWidth = 8;
	
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	int32 InventoryHeight = 5;

protected:
	// 네트워크 환경에서 인벤토리
	UPROPERTY(ReplicatedUsing = OnRep_Slots)
	TArray<FInventorySlot> Slots;
	TArray<FInventorySlot> PrevSlots;
	
	UPROPERTY(BlueprintReadWrite)
	int DragItemIndex = -1;
	
	UPROPERTY(BlueprintReadWrite)
	FItemData CurrentDragItemData;

private:
	UFUNCTION()
	void OnRep_Slots();
		
};
