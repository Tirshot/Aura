// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "EquipmentComponent.generated.h"

struct FEquipmentSlotList;
class AAuraCharacter;
class UAuraAbilitySystemComponent;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemEquipped, const FItemData&, EquippedItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotChanged, int32, Index);

USTRUCT(BlueprintType)
struct FEquipmentSlotEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
public:
	FEquipmentSlotEntry() 
	{
		ItemSubGroup = EItemSubGroup::None;
		ItemData = FItemData();
		bIsSlotEquipped = false;
	}
	
	FEquipmentSlotEntry(EItemSubGroup InSubGroup) : ItemSubGroup(InSubGroup) {}
	FEquipmentSlotEntry(EItemSubGroup InSubGroup, FItemData InItemData) : ItemSubGroup(InSubGroup), ItemData(InItemData), bIsSlotEquipped(true) {}
	
public:
	UPROPERTY()
	int32 SlotID = -1;
	
	UPROPERTY()
	EItemSubGroup ItemSubGroup;
	
	// 아이템 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemData ItemData;

	UPROPERTY()
	bool bIsSlotEquipped = false;
	
	bool IsEmpty() const { return ItemData.Name.IsNone(); }
	
	void PostReplicatedAdd(const FEquipmentSlotList& InArraySerializer);
	void PostReplicatedChange(const FEquipmentSlotList& InArraySerializer);
	void PreReplicatedRemove(const FEquipmentSlotList& InArraySerializer);
};

USTRUCT(BlueprintType)
struct FEquipmentSlotList : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FEquipmentSlotEntry> Items;

	UPROPERTY()
	class UEquipmentComponent* OwnerComponent = nullptr;
	
	void EquipmentSlotChanged(FEquipmentSlotEntry& Slot);

	// 직렬화
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FEquipmentSlotEntry, FEquipmentSlotList>(Items, DeltaParms, *this);
	}
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
	void ClearSlot(FEquipmentSlotEntry* Slot);
	
	FEquipmentSlotEntry* GetSlotEntry(EItemSubGroup Slot);
	
	UFUNCTION(BlueprintCallable)
	void SetItemDataToEquipmentSlotViewModels();
	void SetItemDataToEquipmentSlotViewModel(EItemSubGroup Slot);

	// 저장용, 슬롯 맵 가져오기
	FEquipmentSlotList& GetSlots(){ return EquipmentSlots; }
	
	// 불러오기용, 슬롯 채우기
	void SetEquipmentSlots(FEquipmentSlotList SavedEquipmentMap);
	void ApplyItemStat(const FItemData& ItemData, FEquipmentSlotEntry* Slot);
	void ReEquipItem();
	
	// 아이템 메시 캐릭터에 장착, 해제
	void AttachItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData, AAuraCharacter* AuraCharacter, EItemSubGroup ItemGroup, FName SocketName);
	void AttachBootsItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData, AAuraCharacter* AuraCharacter);
	
	void AttachItemMeshToAuraCharacterMesh(const FItemData& ItemData, AAuraCharacter* AuraCharacter);
	void DetachItemMeshFromAuraCharacterMesh(EItemSubGroup ItemSubGroup);
	
public:
	UPROPERTY(BlueprintAssignable, BlueprintReadOnly)
	FOnItemEquipped OnItemEquipped;
	
	UPROPERTY()
	FOnEquipmentSlotChanged OnEquipmentSlotChanged;
	
protected:
	// 내부 장착
	void EquipItem_Internal(const FItemData& ItemData, int OriginIndex);
	
	// 내부 장착 해제
	void UnEquipItem_Internal(FEquipmentSlotEntry* Slot);
	
public:
	// 슬롯 타입과 슬롯 정보를 연결
	UPROPERTY(Replicated)
	FEquipmentSlotList EquipmentSlots;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> AttachedMesh;
	
	UPROPERTY()
	UInventoryComponent* InventoryComponent;

	UPROPERTY()
	UAuraAbilitySystemComponent* AuraASC;
};

// FastArray 등록
template<>
struct TStructOpsTypeTraits<FEquipmentSlotList> : public TStructOpsTypeTraitsBase2<FEquipmentSlotList>
{
	enum { WithNetDeltaSerializer = true };
};
