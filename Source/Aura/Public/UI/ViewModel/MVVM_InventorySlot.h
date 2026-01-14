// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Player/InventoryComponent.h"
#include "MVVM_InventorySlot.generated.h"

UCLASS()
class AURA_API UMVVM_InventorySlot : public UMVVMViewModelBase
{
	GENERATED_BODY()
	
protected:
	// 인벤토리 컴포넌트
	UPROPERTY()
	UInventoryComponent* Inventory; 
    
	// 이 슬롯 ViewModel이 담당하는 인덱스
	UPROPERTY(BlueprintReadOnly)
	int32 SlotIndex;

public:
	int32 GetSlotIndex() const {return SlotIndex;}
	void SetSlotIndex(const int32 idx) {SlotIndex = idx;}

public:
	UFUNCTION(BlueprintCallable)
	void Initialize(UInventoryComponent* Component, int32 Index);

	UFUNCTION(BlueprintCallable)
	void LoadInitialData();

	UFUNCTION(BlueprintCallable)
	void ClearSlot();
	
public:
	FName GetItemID() const {return ItemID;}
	int32 GetItemCount() const {return ItemCount;}
	FIntPoint GetItemSize() const {return ItemSize;}
	FIntPoint GetStartPoint() const {return StartPoint;}
	bool GetbIsOccupied() const {return bIsOccupied;}
	UTexture2D* GetItemImage() const {return ItemImage;}
	float GetItemImageWidth() const {return ItemImageWidth;}
	float GetItemImageHeight() const {return ItemImageHeight;}
	FText GetItemDescription() const {return ItemDescription;}

	void SetItemID(FName InID);
	void SetItemCount(int32 InCount);
	void SetItemSize(FIntPoint InSize);
	void SetStartPoint(FIntPoint InStartPoint);
	void SetbIsOccupied(bool InBoolean);
	void SetItemImage(UTexture2D* InImage);
	void SetItemImageWidth(float InWidth);
	void SetItemImageHeight(float InHeight);
	void SetItemDescription(FText InDescription);

public:
	UPROPERTY(BlueprintReadWrite)
	FItemData ItemData;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FName ItemID;

	// 아이템 갯수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	int32 ItemCount = 0;

	// 아이템 크기
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FIntPoint ItemSize = FIntPoint::ZeroValue;

	// 아이템 좌상단 시작 위치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FIntPoint StartPoint = FIntPoint::ZeroValue;

	// 아이템 이미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	UTexture2D* ItemImage;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	float ItemImageWidth = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	float ItemImageHeight = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	FText ItemDescription;
	
	// 해당 칸이 점유되었는가
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter)
	bool bIsOccupied = false; 
};
