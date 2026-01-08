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

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// 맵의 값을 빈 슬롯 데이터로 채우기
	EquipmentMap.EquipSlotMap.Add(EItemSubGroup::Weapon, FEquipmentSlotInfo());
	EquipmentMap.EquipSlotMap.Add(EItemSubGroup::Helmet, FEquipmentSlotInfo());
	EquipmentMap.EquipSlotMap.Add(EItemSubGroup::Armor, FEquipmentSlotInfo());
	EquipmentMap.EquipSlotMap.Add(EItemSubGroup::Boots, FEquipmentSlotInfo());
}


void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// ASC 연결
	if (AActor* AvatarActor = GetOwner())
	{
		AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AvatarActor));
		
		// 인벤토리 컴포넌트 연결
		if (AvatarActor->Implements<UPlayerInterface>())
		{
			InventoryComponent = IPlayerInterface::Execute_GetInventoryComponent(AvatarActor);
		}
	}

}

void UEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UEquipmentComponent, EquipmentMap);
	DOREPLIFETIME(UEquipmentComponent, AuraASC);
}


void UEquipmentComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

void UEquipmentComponent::Server_ClearSlot_Implementation(EItemSubGroup Slot)
{
	if (GetOwner()->HasAuthority())
	{
		ClearSlot(GetSlot(Slot));
	}
}

bool UEquipmentComponent::IsSlotEmpty(EItemSubGroup Slot) const
{
	return EquipmentMap.EquipSlotMap.Find(Slot)->IsEmpty();
}

void UEquipmentComponent::ClearSlot(FEquipmentSlotInfo* Slot)
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
	
	// 장착된 메시 제거
	DetachItemMeshFromAuraCharacterMesh(Slot->ItemData.ItemSubGroup);
		
	// 장착 슬롯 비우기
	Slot->ItemData = FItemData();
	Slot->AttachedMesh.Empty();
	Slot->bIsSlotEquipped = false;
	Slot->AbilityTags.Empty();
}

FEquipmentSlotInfo* UEquipmentComponent::GetSlot(EItemSubGroup Slot)
{
	return EquipmentMap.EquipSlotMap.Find(Slot);
}

FEquipmentSlotInfo UEquipmentComponent::GetSlotByCopy(EItemSubGroup Slot)
{
	return EquipmentMap.EquipSlotMap.FindRef(Slot);
}

void UEquipmentComponent::SetEquipmentSlots(TMap<EItemSubGroup, FEquipmentSlotInfo> SavedEquipmentMap)
{
	// 세이브 로드 후 게임에 반영하는 함수
	EquipmentMap.EquipSlotMap = SavedEquipmentMap;
	
	// 1. 아이템 스텟 제거
	if (!AuraASC)
		AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
	
	// 2. 슬롯 뷰 모델을 가져오기
	if (UMVVM_Inventory* InventoryViewModel = UAuraAbilitySystemLibrary::GetInventoryMenuViewModel(this))
	{
		TArray<UMVVM_EquipmentSlot*> EquipSlots = InventoryViewModel->GetAllEquipSlotViewModels();
		if (!EquipSlots.IsEmpty())
		{
			// 3. 각 슬롯에 대해 실제 데이터를 사용해 필드 노티파이 갱신하기
			for (const auto& Pair : EquipmentMap.EquipSlotMap)
			{
				EItemSubGroup ItemSubGroup = Pair.Key;
				FEquipmentSlotInfo SlotInfo = Pair.Value;
				
				if (auto EquipSlotVM = InventoryViewModel->GetEquipSlotViewModel(ItemSubGroup))
				{
					EquipSlotVM->SetItemID(SlotInfo.ItemData.Name);
					EquipSlotVM->SetIcon(SlotInfo.ItemData.Image);
					EquipSlotVM->SetDescription(SlotInfo.ItemData.Description);
					EquipSlotVM->SetbEquipped(SlotInfo.bIsSlotEquipped);
				}
				
				// 4. 아이템 스텟 재적용
				ApplyItemStat(SlotInfo.ItemData, &SlotInfo);
			}
		}
	}
	
	
}

void UEquipmentComponent::ApplyItemStat(const FItemData& ItemData, FEquipmentSlotInfo* Slot)
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
	else if (Slot->ItemData.Name.IsValid())
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
		
		auto* Slot = GetSlot(ItemGroup);
		if (!Slot)
			return;
		
		Slot->AttachedMesh.Add(ItemMesh);
	}
}

void UEquipmentComponent::AttachBootsItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData,
	AAuraCharacter* AuraCharacter)
{
	UStaticMeshComponent* LeftItemMesh = NewObject<UStaticMeshComponent>(GetOwner());
	UStaticMeshComponent* RightItemMesh = NewObject<UStaticMeshComponent>(GetOwner());

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
		
	auto* Slot = GetSlot(EItemSubGroup::Boots);
	if (!Slot)
		return;
		
	Slot->AttachedMesh.Add(LeftItemMesh);
	Slot->AttachedMesh.Add(RightItemMesh);
}

void UEquipmentComponent::AttachItemMeshToAuraCharacterMesh(const FItemData& ItemData, AAuraCharacter* AuraCharacter)
{
	AttachItemMeshToAuraCharacterMesh_Internal(ItemData, AuraCharacter, EItemSubGroup::Helmet, "HelmetSocket");
	AttachItemMeshToAuraCharacterMesh_Internal(ItemData, AuraCharacter, EItemSubGroup::Armor, "ArmorSocket");
	AttachBootsItemMeshToAuraCharacterMesh_Internal(ItemData, AuraCharacter);
}

void UEquipmentComponent::DetachItemMeshFromAuraCharacterMesh(EItemSubGroup ItemSubGroup)
{
	auto* Slot = GetSlot(ItemSubGroup);
	if (!Slot)
		return;
	
	for (const auto& AttachedMesh : Slot->AttachedMesh)
	{
		AttachedMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		AttachedMesh->DestroyComponent();
	}
}

void UEquipmentComponent::EquipItem_Internal(const FItemData& ItemData, int OriginIndex)
{
	if (!AuraASC || !InventoryComponent)
		return;
	
	if (ItemData.ItemGroup != EItemGroup::Equipment)
		return;
	
	// 아이템 데이터에서 데이터 꺼내오기
	EItemSubGroup ItemSubGroup = ItemData.ItemSubGroup;
	
	auto* Slot = GetSlot(ItemSubGroup);
	if (!Slot)
		return;
	
	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(GetOwner());
	if (!AuraCharacter)
		return;
	
	// 아이템 장착 해제
	UnEquipItem_Internal(Slot);
		
	// 스텟 적용
	ApplyItemStat(ItemData, Slot);
	
	// 아이템이 어빌리티 태그들을 가지면 각 어빌리티를 ASC에 부여
	for (const auto TagAndLevel : ItemData.AbilityTagAndLevel)
	{
		const FGameplayTag AbilityTag = TagAndLevel.AbilityTag;
		const int32 AbilityLevel = TagAndLevel.AbilityLevel;
		
		for (int i = 0; i < AbilityLevel; i++)
		{
			AuraASC->AddCharacterAbilityByTag(AbilityTag);
		}
		Slot->bIsSlotEquipped = true;
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
			Slot->bIsSlotEquipped = true;
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
	
	Slot->ItemData = ItemData;
	
	// 슬롯에 장착이 되었다면 기존 아이템은 인벤토리에서 제거
	// 오직 드래그 드랍으로만 수행했을 때만 장착한다고 가정 -> DragOP에서 인덱스 전달
	InventoryComponent->Server_RemoveItemToEquip(OriginIndex);
	
	// 툴팁 제거
	UAuraAbilitySystemLibrary::GetOverlayWidgetController(this)->OnItemToolTipActivated.Broadcast(FName(), false);
	
	// 아이템을 메시에 장착
	AttachItemMeshToAuraCharacterMesh(ItemData, AuraCharacter);
	
	// 아이템 장착 델리게이트 호출
	OnItemEquipped.Broadcast(ItemData);
}

void UEquipmentComponent::UnEquipItem_Internal(FEquipmentSlotInfo* Slot)
{
	if (!AuraASC || !InventoryComponent || !Slot)
		return;
	
	if (Slot->ItemData.ItemGroup != EItemGroup::Equipment)
		return;
	
	// 장착 해제된 아이템 인벤토리에 추가
	InventoryComponent->AddItem_Internal(Slot->ItemData);
	
	// 이미 장착 중인 아이템 효과 해제
	if (Slot->bIsSlotEquipped)
		Server_ClearSlot(Slot->ItemData.ItemSubGroup);
}

void UEquipmentComponent::Server_EquipItem_Implementation(const FItemData& ItemData, int OriginIndex)
{
	if (!GetOwner()->HasAuthority())
		return;
	
	EquipItem_Internal(ItemData, OriginIndex);
}

void UEquipmentComponent::Server_UnequipItem_Implementation(EItemSubGroup Slot)
{
	if (!GetOwner()->HasAuthority())
		return;
	
	auto* EquipSlot = GetSlot(Slot);
	if (!EquipSlot)
		return;
	
	UnEquipItem_Internal(EquipSlot);
}

