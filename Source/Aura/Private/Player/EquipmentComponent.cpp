// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/EquipmentComponent.h"

#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Character/AuraCharacter.h"
#include "Interaction/PlayerInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/AuraPlayerState.h"
#include "Player/InventoryComponent.h"
#include "UI/ViewModel/MVVM_Inventory.h"
#include "UI/WidgetController/OverlayWidgetController.h"

void FEquipmentSlotEntry::PostReplicatedAdd(const struct FEquipmentSlotList& InArraySerializer)
{
	// 슬롯이 처음 로드(추가) 되었을 때 호출
	// UI 갱신
	for (int Index = 0; Index < InArraySerializer.Items.Num(); Index++)
	{
		if (InArraySerializer.OwnerComponent)
		{
			// 2. 델리게이트를 호출하기 전에 반드시 유효성 검사
			if (InArraySerializer.OwnerComponent->OnEquipmentSlotChanged.IsBound())
			{
				InArraySerializer.OwnerComponent->OnEquipmentSlotChanged.Broadcast(Index);
			}
		}
	}
}

void FEquipmentSlotEntry::PostReplicatedChange(const struct FEquipmentSlotList& InArraySerializer)
{
	// 슬롯의 데이터가 변경되었을 때 호출
	// UI 갱신
	for (int Index = 0; Index < InArraySerializer.Items.Num(); Index++)
	{
		if (InArraySerializer.OwnerComponent)
		{
			// 2. 델리게이트를 호출하기 전에 반드시 유효성 검사
			if (InArraySerializer.OwnerComponent->OnEquipmentSlotChanged.IsBound())
			{
				InArraySerializer.OwnerComponent->OnEquipmentSlotChanged.Broadcast(Index);
			}
		}
	}
}

void FEquipmentSlotEntry::PreReplicatedRemove(const struct FEquipmentSlotList& InArraySerializer)
{
	// 슬롯의 데이터가 삭제될 때 호출
	// UI 갱신
	for (int Index = 0; Index < InArraySerializer.Items.Num(); Index++)
	{
		if (InArraySerializer.OwnerComponent)
		{
			// 2. 델리게이트를 호출하기 전에 반드시 유효성 검사
			if (InArraySerializer.OwnerComponent->OnEquipmentSlotChanged.IsBound())
			{
				InArraySerializer.OwnerComponent->OnEquipmentSlotChanged.Broadcast(Index);
			}
		}
	}
}

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}


void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	
	EquipmentSlots.OwnerComponent = this;
	
	// ASC 연결
	if (AActor* AvatarActor = GetOwner())
	{
		AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AvatarActor));
		
		// 인벤토리 컴포넌트 연결
		if (AvatarActor->Implements<UPlayerInterface>())
		{
			InventoryComponent = IPlayerInterface::Execute_GetInventoryComponent(AvatarActor);
		}
		if (EquipmentSlots.Items.IsEmpty())
		{
			// 맵의 값을 빈 슬롯 데이터로 채우기
			EquipmentSlots.Items.Add(FEquipmentSlotEntry(EItemSubGroup::Helmet));
			EquipmentSlots.MarkItemDirty(EquipmentSlots.Items[0]);
			
			EquipmentSlots.Items.Add(FEquipmentSlotEntry(EItemSubGroup::Armor));
			EquipmentSlots.MarkItemDirty(EquipmentSlots.Items[1]);
			
			EquipmentSlots.Items.Add(FEquipmentSlotEntry(EItemSubGroup::Boots));
			EquipmentSlots.MarkItemDirty(EquipmentSlots.Items[2]);
			
			EquipmentSlots.Items.Add(FEquipmentSlotEntry(EItemSubGroup::Weapon));
			EquipmentSlots.MarkItemDirty(EquipmentSlots.Items[3]);
		}
	}
}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UEquipmentComponent, EquipmentSlots);
}


void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UEquipmentComponent::Server_ClearSlot_Implementation(EItemSubGroup Slot)
{
	if (GetOwner()->HasAuthority())
	{
		ClearSlot(GetSlotEntry(Slot));
		
		if (FEquipmentSlotEntry* Entry = GetSlotEntry(Slot))
		{
			ClearSlot(Entry);

			Entry->ItemData = FItemData();
			Entry->bIsSlotEquipped = false;
			EquipmentSlots.MarkItemDirty(*Entry);
		}
	}
}

bool UEquipmentComponent::IsSlotEmpty(EItemSubGroup Slot) const
{
	int SlotIndex = static_cast<uint8>(Slot);
	return EquipmentSlots.Items[SlotIndex].IsEmpty();
}

void UEquipmentComponent::ClearSlot(FEquipmentSlotEntry* Slot)
{
	// 슬롯에 배치된 아이템에 의해 부여된 효과 해제
	AuraASC->RemoveActiveGameplayEffectBySourceEffect(Slot->ItemData.ItemStatEffectClass, nullptr);
	
	// 게임플레이 이펙트 제거
	for (const auto EffectAndStack : Slot->ItemData.EffectAndStacks)
	{
		AuraASC->RemoveActiveGameplayEffectBySourceEffect(EffectAndStack.EffectClass, nullptr);
	}
	
	// 슬롯에 배치된 아이템에 의해 부여된 어빌리티 제거
	for (const auto TagAndLevel : Slot->ItemData.AbilityTagAndLevel)
	{
		const FGameplayTag AbilityTag = TagAndLevel.AbilityTag;
		const int32 AbilityLevel = TagAndLevel.AbilityLevel;
		
		AuraASC->RemoveCharacterAbilityByTag(AbilityTag, AbilityLevel);
	}
	
	if (AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(GetOwner()))
	{
		if (AAuraPlayerState* AuraPS = AuraCharacter->GetPlayerState<AAuraPlayerState>())
		{
			// 어빌리티 업그레이드 제거
			for (const auto Pair : Slot->ItemData.AbilityUpgradeTagAndLevel)
			{
				// 부여하고자 하는 레벨만큼 반복
				for (int i = 0; i < Pair.AbilityLevel; i++)
				{
					AuraPS->Server_RemoveAbilityUpgradeTag(Pair.AbilityTag);
				}
		
			}
		}
	}
	
	// 장착된 메시 제거
	DetachItemMeshFromAuraCharacterMesh(Slot->ItemData.ItemSubGroup);
}

FEquipmentSlotEntry* UEquipmentComponent::GetSlotEntry(EItemSubGroup Slot)
{
	for (auto& Entry : EquipmentSlots.Items)
	{
		if (Entry.ItemSubGroup == Slot)
		{
			return &Entry;
		}
	}
	return nullptr;
}

void UEquipmentComponent::SetItemDataToEquipmentSlotViewModels()
{
	if (UMVVM_Inventory* InventoryViewModel = UAuraAbilitySystemLibrary::GetInventoryMenuViewModel(this))
	{
		// 각 슬롯에 대해 실제 데이터를 사용해 필드 노티파이 갱신하기
		for (const auto& SlotEntry : EquipmentSlots.Items)
		{
			const EItemSubGroup& ItemSubGroup = SlotEntry.ItemSubGroup;
			const FItemData& SlotItemData = SlotEntry.ItemData;
				
			if (auto* EquipSlotVM = InventoryViewModel->GetEquipSlotViewModel(ItemSubGroup))
			{
				EquipSlotVM->SetItemID(SlotItemData.Name);
				EquipSlotVM->SetIcon(SlotItemData.Image.Get());
				EquipSlotVM->SetDescription(SlotItemData.Description);
				EquipSlotVM->SetbEquipped(SlotEntry.bIsSlotEquipped);
			}
		}
		
	}
}

void UEquipmentComponent::SetItemDataToEquipmentSlotViewModel(EItemSubGroup Slot)
{
	if (UMVVM_Inventory* InventoryViewModel = UAuraAbilitySystemLibrary::GetInventoryMenuViewModel(this))
	{
		TArray<UMVVM_EquipmentSlot*> EquipSlots = InventoryViewModel->GetAllEquipSlotViewModels();
		if (!EquipSlots.IsEmpty())
		{
			auto Slots = GetSlots();
			const auto& SlotInfo = GetSlotEntry(Slot);
			
			if (auto EquipSlotVM = InventoryViewModel->GetEquipSlotViewModel(Slot))
			{
				EquipSlotVM->ReInitializeSlotView(SlotInfo->ItemData);
			}
		}
	}
}

void UEquipmentComponent::SetEquipmentSlots(FEquipmentSlotList SavedEquipmentMap)
{
	// 세이브 로드 후 게임에 반영하는 함수
	EquipmentSlots = SavedEquipmentMap;
	
	if (!InventoryComponent)
		InventoryComponent = IPlayerInterface::Execute_GetInventoryComponent(GetOwner());
	
	if (!AuraASC)
		AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
	
	for (const auto& SlotEntry : SavedEquipmentMap.Items)
	{
		const auto& ItemData = SlotEntry.ItemData;
				
		if (ItemData.Name == "")
			continue;
		
		// 아이템 장착
		EquipItem_Internal(ItemData, -1);
	}
}

void UEquipmentComponent::ApplyItemStat(const FItemData& ItemData, FEquipmentSlotEntry* Slot)
{
	FItemStat ItemStat = ItemData.ItemStat;
	if (!ItemStat.IsEmpty())
	{
		if (!AuraASC)
			AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
				
		FGameplayEffectContextHandle Context = AuraASC->MakeEffectContext();
		Context.AddInstigator(AuraASC->GetAvatarActor(), nullptr);
		
		auto SpecHandle = AuraASC->MakeOutgoingSpec(ItemData.ItemStatEffectClass, 1.f, Context);
		
		if (SpecHandle.IsValid())
		{
			const float Str = ItemStat.Strength;
			const float Int = ItemStat.Intelligence;
			const float Res = ItemStat.Resilience;
			const float Vig = ItemStat.Vigor;
			const float MoveSpeed = ItemStat.MovementSpeed;
			const float MaxHP = ItemStat.MaxHealth;
			const float MaxMP = ItemStat.MaxMana;
			const float MAP = ItemStat.MagicAttackPower;
			const float Armor = ItemStat.Armor;
			const float ArmorPenet = ItemStat.ArmorPenetration;
			const float HealthRegen = ItemStat.HealthRegeneration;
			const float ManaRegen = ItemStat.ManaRegeneration;
			const float CriticalChance = ItemStat.CriticalHitChance;
		
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Primary_Strength, Str);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Primary_Intelligence, Int);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Primary_Resilience, Res);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Primary_Vigor, Vig);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_MovementSpeed, MoveSpeed);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth, MaxHP);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_MaxMana, MaxMP);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower, MAP);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_Armor, Armor);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_ArmorPenetration, ArmorPenet);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_HealthRegeneration, HealthRegen);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_ManaRegeneration, ManaRegen);
			SpecHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Secondary_CriticalHitChance, CriticalChance);
		
			AuraASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
			
			Slot->bIsSlotEquipped = true;
		}
	}
	else if (ItemData.Name.IsValid())
	{
		Slot->bIsSlotEquipped = true;
	}
}

void UEquipmentComponent::AttachItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData, AAuraCharacter* AuraCharacter, EItemSubGroup ItemGroup, FName SocketName)
{
	if (ItemData.ItemSubGroup == ItemGroup)
	{
		UStaticMeshComponent* ItemMesh = NewObject<UStaticMeshComponent>(GetOwner());

		ItemMesh->SetStaticMesh(ItemData.StaticMesh);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemMesh->SetCanEverAffectNavigation(false);
		ItemMesh->RegisterComponent();

		ItemMesh->AttachToComponent(
			AuraCharacter->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SocketName);
		
		AttachedMesh.Add(ItemMesh);
	}
}

void UEquipmentComponent::AttachBootsItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData,
	AAuraCharacter* AuraCharacter)
{
	UStaticMeshComponent* LeftItemMesh = NewObject<UStaticMeshComponent>(AuraCharacter);
	UStaticMeshComponent* RightItemMesh = NewObject<UStaticMeshComponent>(AuraCharacter);
	
	RightItemMesh->SetStaticMesh(ItemData.RightFootMesh);
	RightItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightItemMesh->SetCanEverAffectNavigation(false);
	RightItemMesh->RegisterComponent();
		
	RightItemMesh->AttachToComponent(
		AuraCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		FName("RightFootSocket"));
		
	LeftItemMesh->SetStaticMesh(ItemData.LeftFootMesh);
	LeftItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftItemMesh->SetCanEverAffectNavigation(false);
	LeftItemMesh->RegisterComponent();
	
	LeftItemMesh->AttachToComponent(
		AuraCharacter->GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		FName("LeftFootSocket"));
	
	AttachedMesh.Add(LeftItemMesh);
	AttachedMesh.Add(RightItemMesh);
}

void UEquipmentComponent::AttachItemMeshToAuraCharacterMesh(const FItemData& ItemData, AAuraCharacter* AuraCharacter)
{
	AttachItemMeshToAuraCharacterMesh_Internal(ItemData, AuraCharacter, EItemSubGroup::Helmet, "HelmetSocket");
	AttachItemMeshToAuraCharacterMesh_Internal(ItemData, AuraCharacter, EItemSubGroup::Armor, "ArmorSocket");
	AttachBootsItemMeshToAuraCharacterMesh_Internal(ItemData, AuraCharacter);
}

void UEquipmentComponent::DetachItemMeshFromAuraCharacterMesh(EItemSubGroup ItemSubGroup)
{
	auto Slot = GetSlotEntry(ItemSubGroup);
	if (!Slot)
		return;
	
	// for (const auto& Mesh : AttachedMesh)
	// {
	// 	if (Mesh->GetStaticMesh())
	// 	{
	// 		Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	// 		Mesh->DestroyComponent();
	// 		return;
	// 	}
	// }
}

void UEquipmentComponent::EquipItem_Internal(const FItemData& ItemData, int OriginIndex)
{
	if (!AuraASC || !InventoryComponent)
		return;
	
	if (ItemData.ItemGroup != EItemGroup::Equipment)
		return;
	
	// 아이템 데이터에서 데이터 꺼내오기
	EItemSubGroup ItemSubGroup = ItemData.ItemSubGroup;
	
	auto* SlotEntry = GetSlotEntry(ItemSubGroup);
	if (!SlotEntry)
		return;
	
	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(GetOwner());
	if (!AuraCharacter)
		return;
	
	// 해당 슬롯에 기존에 장착된 아이템 장착 해제
	// Index == -1, 저장 데이터에서 가져옴(아이템 추가할 필요 없음)
	if (OriginIndex == -1)
	{
		// 아이템 장착 해제
		Server_ClearSlot(ItemSubGroup);
	}
	else
	{
		// 아이템 장착 해제, 인벤토리에 추가
		Server_UnequipItem(ItemSubGroup);
	}
		
	// 스텟 적용
	ApplyItemStat(ItemData, SlotEntry);
	
	// 아이템이 어빌리티 태그들을 가지면 각 어빌리티를 ASC에 부여
	for (const auto TagAndLevel : ItemData.AbilityTagAndLevel)
	{
		const FGameplayTag AbilityTag = TagAndLevel.AbilityTag;
		const int32 AbilityLevel = TagAndLevel.AbilityLevel;
		
		for (int i = 0; i < AbilityLevel; i++)
		{
			AuraASC->AddCharacterAbilityByTag(AbilityTag);
		}
		SlotEntry->bIsSlotEquipped = true;
	}
	
	// 아이템이 게임플레이 이펙트 핸들을 가지면 이펙트를 ASC에 적용
	for (const auto EffectAndStack : ItemData.EffectAndStacks)
	{
		if (EffectAndStack.EffectClass)
		{
			const FGameplayEffectContextHandle Context = AuraASC->MakeEffectContext();
			const auto SpecHandle = AuraASC->MakeOutgoingSpec(EffectAndStack.EffectClass, EffectAndStack.EffectStack, Context);
			AuraASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		
			// 장착 중
			SlotEntry->bIsSlotEquipped = true;
		}
	}
	
	// 어빌리티 업그레이드 태그 추가
	// PlayerState에 접근 -> 어빌리티 업그레이드 태그 추가
	for (const auto Pair : ItemData.AbilityUpgradeTagAndLevel)
	{
		if (AAuraPlayerState* AuraPS = AuraCharacter->GetPlayerState<AAuraPlayerState>())
		{
			// 부여하고자 하는 레벨만큼 반복
			for (int i = 0; i < Pair.AbilityLevel; i++)
			{
				AuraPS->Server_AddAbilityUpgradeTag(Pair.AbilityTag);
			}
		}
	}
	
	// 슬롯에 장착이 되었다면 기존 아이템은 인벤토리에서 제거
	// DragOP에서 인덱스 전달, -1 == 인벤토리에서 장착하지 않는 경우
	if (OriginIndex >= 0)
		InventoryComponent->Server_RemoveItemToEquip(OriginIndex);
	
	SlotEntry->ItemData = ItemData;
	SlotEntry->bIsSlotEquipped = true;
	
	// UI 반영
	uint8 ItemIndex = static_cast<uint8>(ItemData.ItemSubGroup);
	OnEquipmentSlotChanged.Broadcast(ItemIndex);
	
	// 툴팁 제거
	UAuraAbilitySystemLibrary::GetOverlayWidgetController(this)->OnItemToolTipActivated.Broadcast(FItemData(), false);
	
	// 아이템을 메시에 장착
	AttachItemMeshToAuraCharacterMesh(ItemData, AuraCharacter);
		
	// 아이템 장착 델리게이트 호출
	OnItemEquipped.Broadcast(ItemData);
}

void UEquipmentComponent::UnEquipItem_Internal(FEquipmentSlotEntry* Slot)
{
	if (!AuraASC || !InventoryComponent || !Slot)
		return;
	
	if (Slot->ItemData.ItemGroup != EItemGroup::Equipment)
		return;
	
	if (Slot->bIsSlotEquipped)
	{
		EItemSubGroup SubGroup = Slot->ItemData.ItemSubGroup;
		
		// 장착 해제된 아이템 인벤토리에 추가
		InventoryComponent->AddItem_Internal(Slot->ItemData);
		
		// 이미 장착 중인 아이템 효과 해제
		Server_ClearSlot(SubGroup);
	}
}

void UEquipmentComponent::Server_EquipItem_Implementation(const FItemData& ItemData, int OriginIndex)
{
	if (!GetOwner()->HasAuthority())
		return;
	
	EquipItem_Internal(ItemData, OriginIndex);
	
	for (FEquipmentSlotEntry& Entry : EquipmentSlots.Items)
	{
		if (Entry.ItemSubGroup == ItemData.ItemSubGroup)
		{
			Entry.ItemData = ItemData;
			Entry.bIsSlotEquipped = true;
            
			EquipmentSlots.MarkItemDirty(Entry); 
			break;
		}
	}
}

void UEquipmentComponent::Server_UnequipItem_Implementation(EItemSubGroup Slot)
{
	if (!GetOwner()->HasAuthority())
		return;
	
	auto EquipSlot = GetSlotEntry(Slot);
	if (!EquipSlot->IsEmpty())
	{
		UnEquipItem_Internal(EquipSlot);
	}
}

