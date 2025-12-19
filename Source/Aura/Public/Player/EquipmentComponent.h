// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "GameplayAbilitySpecHandle.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Components/ActorComponent.h"
#include "EquipmentComponent.generated.h"

class UAuraAbilitySystemComponent;
class UInventoryComponent;

// 슬롯에 적용되는 데이터
USTRUCT(BlueprintType)
struct FEquipmentSlotInfo
{
	GENERATED_BODY()

public:
	// 아이템 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemData ItemData;

	// 아이템으로 획득하는 어빌리티
	UPROPERTY()
	TArray<FGameplayTag> AbilityTags;

	UPROPERTY()
	bool bIsSlotEquipped = false;
	
	bool IsEmpty() const { return ItemData.Name.IsNone(); }
};

USTRUCT(BlueprintType)
struct FEquipSlotMap
{
	GENERATED_BODY()
	
public:
	FEquipmentSlotInfo* GetEquipmentSlot(EItemSubGroup SubGroup) {return EquipSlotMap.Find(SubGroup);}
	
	//UPROPERTY(BlueprintReadOnly)
	TMap<EItemSubGroup, FEquipmentSlotInfo> EquipSlotMap;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class AURA_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UEquipmentComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_EquipItem(const FItemData& ItemData, int OriginIndex);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UnequipItem(EItemSubGroup Slot);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_ClearSlot(EItemSubGroup Slot);
	
	// 슬롯 비었는지 확인
	bool IsSlotEmpty(EItemSubGroup Slot) const;
	
	// 슬롯 비우기
	void ClearSlot(FEquipmentSlotInfo* Slot);

	// 슬롯에 들어있는 아이템 가져오기
	FEquipmentSlotInfo* GetSlot(EItemSubGroup Slot);
	
	UFUNCTION(BlueprintCallable)
	FEquipmentSlotInfo GetSlotByCopy(EItemSubGroup Slot);
	
	// 저장용, 슬롯 맵 가져오기
	FEquipSlotMap GetSlots(){ return EquipmentMap; }
	
	// 불러오기용, 슬롯 채우기
	void SetEquipmentSlots(TMap<EItemSubGroup, FEquipmentSlotInfo> SavedEquipmentMap);
	void ApplyItemStat(const FItemData& ItemData, FEquipmentSlotInfo* Slot);

protected:
	// 내부 장착
	void EquipItem_Internal(const FItemData& ItemData, int OriginIndex);
	
	// 내부 장착 해제
	void UnEquipItem_Internal(FEquipmentSlotInfo* Slot);
	
public:
	// 슬롯 타입과 슬롯 정보를 연결
	UPROPERTY(Replicated)
	FEquipSlotMap EquipmentMap;

	UPROPERTY()
	UInventoryComponent* InventoryComponent;

	UPROPERTY()
	UAuraAbilitySystemComponent* AuraASC;
		
};
