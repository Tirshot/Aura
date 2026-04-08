// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "UI/ViewModel/MVVM_Inventory.h"
#include "EquipmentComponent.generated.h"

struct FEquipmentSlotList;
class AAuraCharacter;
class UAuraAbilitySystemComponent;
class UInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemEquipped, const FItemData&, EquippedItemData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentSlotChanged, int32, Index);

USTRUCT(BlueprintType)
struct FEquipmentSlot : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
public:
	FEquipmentSlot() 
	{
		ItemSubGroup = EItemSubGroup::None;
		ItemData = FItemData();
		bIsSlotEquipped = false;
	}
	
	FEquipmentSlot(EItemSubGroup InSubGroup) : ItemSubGroup(InSubGroup) {}
	FEquipmentSlot(EItemSubGroup InSubGroup, FItemData InItemData) : ItemSubGroup(InSubGroup), ItemData(InItemData), bIsSlotEquipped(true) {}
	
public:
	UPROPERTY()
	int32 SlotID = -1;
	
	UPROPERTY()
	EItemSubGroup ItemSubGroup;
	
	// 아이템 데이터
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FItemData ItemData;

	UPROPERTY()
	class UEquipmentComponent* Equipment;
	
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
	TArray<FEquipmentSlot> Items;

	UPROPERTY()
	class UEquipmentComponent* OwnerComponent = nullptr;
	
	void EquipmentSlotChanged(FEquipmentSlot& Slot);

	// 직렬화
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FEquipmentSlot, FEquipmentSlotList>(Items, DeltaParms, *this);
	}
};


// FastArray 등록
template<>
struct TStructOpsTypeTraits<FEquipmentSlotList> : public TStructOpsTypeTraitsBase2<FEquipmentSlotList>
{
	enum { WithNetDeltaSerializer = true };
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
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	
public:	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_EquipItem(const FItemData& ItemData, int OriginIndex);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_UnequipItem(EItemSubGroup Slot);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_ClearSlot(EItemSubGroup Slot);
	
public:
	void InitializeEquipmentSlots();
	void ForceReplication();
	
	// 슬롯 비었는지 확인
	bool IsSlotEmpty(EItemSubGroup Slot) const;

	// 슬롯 비우기
	void ClearSlot(FEquipmentSlot* Slot);
	
	FEquipmentSlot* GetSlotEntry(EItemSubGroup Slot);
	
	UFUNCTION(BlueprintCallable)
	void SetItemDataToEquipmentSlotViewModels();
	void SetItemDataToEquipmentSlotViewModel(EItemSubGroup Slot);

	// 저장용, 슬롯 맵 가져오기
	FEquipmentSlotList& GetSlots(){ return EquipmentSlots; }
	
	// 불러오기용, 슬롯 채우기
	void SetEquipmentSlots(const FEquipmentSlotList& SavedEquipmentMap);
	void ApplyItemStat(const FItemData& ItemData, FEquipmentSlot* Slot);
	void ReEquipItem();
	
	// 아이템 메시 캐릭터에 장착, 해제
	void AttachItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData, AAuraCharacter* AuraCharacter, EItemSubGroup ItemGroup, FName SocketName);
	void AttachBootsItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData, AAuraCharacter* AuraCharacter);
	
	void AttachItemMeshToAuraCharacterMesh(const FItemData& ItemData, AAuraCharacter* AuraCharacter);
	void DetachItemMeshFromAuraCharacterMesh(EItemSubGroup ItemSubGroup);
	
protected:
	// 내부 장착
	void EquipItem_Internal(const FItemData& ItemData, int OriginIndex);
	
	// 내부 장착 해제
	void UnEquipItem_Internal(FEquipmentSlot* Slot);
	
public:
	UPROPERTY(BlueprintAssignable, BlueprintReadOnly)
	FOnItemEquipped OnItemEquipped;
	
	UPROPERTY()
	FOnEquipmentSlotChanged OnEquipmentSlotChanged;
	
	UFUNCTION()
	void HUDInitialized();
	
	UFUNCTION()
	void OnCharacterInitialized(ACharacter* AvatarCharacter);
	
public:
	// 슬롯 타입과 슬롯 정보를 연결
	UPROPERTY(Replicated)
	FEquipmentSlotList EquipmentSlots;
	
	UPROPERTY()
	UMVVM_Inventory* InventoryViewModel;

	UPROPERTY()
	TMap<EItemSubGroup, TObjectPtr<UStaticMeshComponent>> AttachedMesh;
	
	UPROPERTY()
	UInventoryComponent* InventoryComponent;

	UPROPERTY()
	UAuraAbilitySystemComponent* AuraASC;
};
