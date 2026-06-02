// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/EquipmentComponent.h"

#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Character/AuraCharacter.h"
#include "Game/AuraGameInstance.h"
#include "Net/UnrealNetwork.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"
#include "Player/InventoryComponent.h"
#include "UI/HUD/AuraHUD.h"
#include "UI/ViewModel/MVVM_Inventory.h"
#include "UI/WidgetController/OverlayWidgetController.h"

void FEquipmentSlot::PostReplicatedAdd(const FEquipmentSlotList& InArraySerializer)
{
	if (!Equipment)
		return;
	
	if (Equipment->OnEquipmentSlotChanged.IsBound())
		Equipment->OnEquipmentSlotChanged.Broadcast(SlotID);
}

void FEquipmentSlot::PostReplicatedChange(const FEquipmentSlotList& InArraySerializer)
{
	if (!Equipment)
		return;
	
	if (InArraySerializer.OwnerComponent->OnEquipmentSlotChanged.IsBound())
		InArraySerializer.OwnerComponent->OnEquipmentSlotChanged.Broadcast(SlotID);
}

void FEquipmentSlot::PreReplicatedRemove(const FEquipmentSlotList& InArraySerializer)
{
}

void FEquipmentSlotList::EquipmentSlotChanged(FEquipmentSlot& Slot)
{
	if (OwnerComponent && OwnerComponent->OnEquipmentSlotChanged.IsBound())
		OwnerComponent->OnEquipmentSlotChanged.Broadcast(Slot.SlotID);
}

UEquipmentComponent::UEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
	EquipmentSlots.OwnerComponent = this;
}


void UEquipmentComponent::Client_ApplyHPMPRatio_Implementation(float NewHP, float NewMP)
{	
	ApplyHPMPRatio(NewHP, NewMP);
}

void UEquipmentComponent::InitializeEquipmentSlots()
{
	// 맵의 값을 빈 슬롯 데이터로 채우기
	EquipmentSlots.Items.Add(FEquipmentSlot(EItemSubGroup::Helmet));
	EquipmentSlots.Items[0].SlotID = 0;
	EquipmentSlots.Items[0].Equipment = this;
	EquipmentSlots.MarkItemDirty(EquipmentSlots.Items[0]);
	EquipmentSlots.EquipmentSlotChanged(EquipmentSlots.Items[0]);
			
	EquipmentSlots.Items.Add(FEquipmentSlot(EItemSubGroup::Armor));
	EquipmentSlots.Items[1].SlotID = 1;
	EquipmentSlots.Items[1].Equipment = this;
	EquipmentSlots.MarkItemDirty(EquipmentSlots.Items[1]);
	EquipmentSlots.EquipmentSlotChanged(EquipmentSlots.Items[1]);
			
	EquipmentSlots.Items.Add(FEquipmentSlot(EItemSubGroup::Boots));
	EquipmentSlots.Items[2].SlotID = 2;
	EquipmentSlots.Items[2].Equipment = this;
	EquipmentSlots.MarkItemDirty(EquipmentSlots.Items[2]);
	EquipmentSlots.EquipmentSlotChanged(EquipmentSlots.Items[2]);
			
	EquipmentSlots.Items.Add(FEquipmentSlot(EItemSubGroup::Weapon));
	EquipmentSlots.Items[3].SlotID = 3;
	EquipmentSlots.Items[3].Equipment = this;
	EquipmentSlots.MarkItemDirty(EquipmentSlots.Items[3]);
	EquipmentSlots.EquipmentSlotChanged(EquipmentSlots.Items[3]);
	
	if (auto* AuraPS = Cast<AAuraPlayerState>(GetOwner()))
	{
		InventoryComponent = UAuraAbilitySystemLibrary::GetInventoryComponentByPlayerState(AuraPS);
	}
}

void UEquipmentComponent::ForceReplication()
{
	if (GetOwnerRole() != ROLE_Authority)
		return;
	
	EquipmentSlots.MarkArrayDirty();
	
	GetOwner()->ForceNetUpdate();
}

void UEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
	
	AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner());
	if (!AuraPS)
		return;
	
	// 인벤토리 컴포넌트, ASC 연결
	InventoryComponent = AuraPS->GetInventoryComponent();
	AuraASC = Cast<UAuraAbilitySystemComponent>(AuraPS->GetAbilitySystemComponent());
	if (!AuraPS->HasAuthority())
		return;
	
	if (EquipmentSlots.Items.IsEmpty())
	{
		InitializeEquipmentSlots();
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
	if (FEquipmentSlot* Entry = GetSlotEntry(Slot))
	{
		ClearSlot(Entry);

		// Entry->ItemData = FItemData();
		Entry->bIsSlotEquipped = false;
		EquipmentSlots.MarkItemDirty(*Entry);
		EquipmentSlots.EquipmentSlotChanged(*Entry);
	}
}

bool UEquipmentComponent::IsSlotEmpty(EItemSubGroup Slot) const
{
	int SlotIndex = static_cast<uint8>(Slot);
	return EquipmentSlots.Items[SlotIndex].IsEmpty();
}

void UEquipmentComponent::ClearSlot(FEquipmentSlot* Slot)
{
	float OldMaxHP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth);
	float OldHP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Vital_Health);
	float OldMaxMP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxMana);
	float OldMP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Vital_Mana);
	float HPRatio = (OldMaxHP > 0.f) ? (OldHP / OldMaxHP) : 1.f;
	float MPRatio = (OldMaxMP > 0.f) ? (OldMP / OldMaxMP) : 1.f;
	
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
	
	// 비율에 따라 HP 감소시키기
	float CurrentHP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Vital_Health);
	float CurrentMP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Vital_Mana);
	
	// 비율에 따라 HP/MP 목표치 계산 및 Delta 구하기
	float NewMaxHP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth);
	float TargetHP = HPRatio * NewMaxHP;
	float HPDelta = TargetHP - CurrentHP;
	
	float NewMaxMP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxMana);
	float TargetMP = MPRatio * NewMaxMP;
	float MPDelta = TargetMP - CurrentMP;
	
	if (HPDelta != 0.f || MPDelta != 0.f)
	{
		// 클라이언트 RPC
		// Client_ApplyHPMPRatio(HPDelta, MPDelta);
				
		if (GetOwnerRole() == ROLE_Authority)
		{
			// 리슨 서버일 때 적용
			ApplyHPMPRatio(HPDelta, MPDelta);
		}
	}
	
	// 장착된 메시 제거
	DetachItemMeshFromAuraCharacterMesh(Slot->ItemData.ItemSubGroup);
	Slot->ItemData = FItemData();
	Slot->bIsSlotEquipped = false;
	GetSlots().MarkArrayDirty();
}

FEquipmentSlot* UEquipmentComponent::GetSlotEntry(EItemSubGroup Slot)
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
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetOwner()))
	{
		if (PC->IsLocalController())
		{
			if (AAuraHUD* AuraHUD = PC->GetHUD<AAuraHUD>())
			{
				FWidgetControllerParams WCParams;
				WCParams.PlayerController = PC;
				WCParams.AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PC->GetPawn());
				
				if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
					WCParams.PlayerState = PlayerState;
				
				if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(WCParams.PlayerState))
				{
					WCParams.AttributeSet = AuraPS->GetAttributeSet();
				}
				
				InventoryViewModel = AuraHUD->GetInventoryViewModel(WCParams);
				if (InventoryViewModel)
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
		}
	}
}

void UEquipmentComponent::SetItemDataToEquipmentSlotViewModel(EItemSubGroup Slot)
{
	if (APlayerController* PC = Cast<APlayerController>(GetOwner()->GetOwner()))
	{
		if (PC->IsLocalController())
		{
			if (AAuraHUD* AuraHUD = PC->GetHUD<AAuraHUD>())
			{
				FWidgetControllerParams WCParams;
				WCParams.PlayerController = PC;
				WCParams.AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PC->GetPawn());
				
				if (APlayerState* PlayerState = Cast<APlayerState>(GetOwner()))
					WCParams.PlayerState = PlayerState;
				
				if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(WCParams.PlayerState))
				{
					WCParams.AttributeSet = AuraPS->GetAttributeSet();
				}
				InventoryViewModel = AuraHUD->GetInventoryViewModel(WCParams);
				if (InventoryViewModel)
				{
					TArray<UMVVM_EquipmentSlot*> EquipSlots = InventoryViewModel->GetAllEquipSlotViewModels();
					if (!EquipSlots.IsEmpty())
					{
						const auto& SlotInfo = GetSlotEntry(Slot);
			
						if (auto EquipSlotVM = InventoryViewModel->GetEquipSlotViewModel(Slot))
						{
							EquipSlotVM->ReInitializeSlotView(SlotInfo->ItemData);
						}
					}
				}
			}
		}
	}
}

void UEquipmentComponent::SetEquipmentSlots(const FEquipmentSlotList& SavedEquipmentMap)
{
	// 서버가 아니면 리턴
	if (GetOwnerRole() != ROLE_Authority)
		return;
	
	EquipmentSlots.Items.Empty();
	EquipmentSlots.OwnerComponent = this;
	EquipmentSlots.MarkArrayDirty();
	
	for (const FEquipmentSlot& Slot : SavedEquipmentMap.Items)
	{
		FEquipmentSlot& NewSlot = EquipmentSlots.Items.Add_GetRef(Slot);
		NewSlot.Equipment = this;
		EquipmentSlots.MarkItemDirty(NewSlot);
	}
	
	ReEquipItem();
	
	AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner());
	if (!AuraPS)
		return;
	
	if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(AuraPS->GetPlayerController()))
	{
		if (!AuraPC->OnCharacterInit.IsAlreadyBound(this, &UEquipmentComponent::OnCharacterInitialized))
			AuraPC->OnCharacterInit.AddDynamic(this, &UEquipmentComponent::OnCharacterInitialized);
	}
	
	if (!InventoryComponent)
		InventoryComponent = UAuraAbilitySystemLibrary::GetInventoryComponentByPlayerState(AuraPS);
	
	if (!AuraASC)
		AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
}

void UEquipmentComponent::ApplyItemStat(const FItemData& ItemData, FEquipmentSlot* Slot)
{
	FItemStat ItemStat = ItemData.ItemStat;
	if (!ItemStat.IsEmpty())
	{
		if (!AuraASC)
			AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
				
		FGameplayEffectContextHandle Context = AuraASC->MakeEffectContext();
		Context.AddInstigator(GetOwner(), nullptr);
		
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
			
			float OldMaxHP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth);
			float OldHP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Vital_Health);
			float OldMaxMP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxMana);
			float OldMP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Vital_Mana);

			float HPRatio = (OldMaxHP > 0.f) ? (OldHP / OldMaxHP) : 1.f;
			float MPRatio = (OldMaxMP > 0.f) ? (OldMP / OldMaxMP) : 1.f;
			
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
			
			float NewMaxHP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth);
			float TargetHP = HPRatio * NewMaxHP;
			float DeltaHP = TargetHP - OldHP;
			
			float NewMaxMP = UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxMana);
			float TargetMP = MPRatio * NewMaxMP;
			float DeltaMP = TargetMP - OldMP;
			
			if (DeltaHP != 0.f || DeltaMP != 0.f)
			{
				// 클라이언트 RPC
				// Client_ApplyHPMPRatio(DeltaHP, DeltaMP);
				
				if (GetOwnerRole() == ROLE_Authority)
				{
					// 리슨 서버일 때 적용
					ApplyHPMPRatio(DeltaHP, DeltaMP);
				}
			}
			Slot->bIsSlotEquipped = true;
		}
	}
	else if (ItemData.Name.IsValid())
	{
		Slot->bIsSlotEquipped = true;
	}
}

void UEquipmentComponent::ReEquipItem()
{
	for (const auto& Slot: EquipmentSlots.Items)
	{
		const auto& ItemData = Slot.ItemData;
				
		if (ItemData.Name == "")
			continue;
		
		// 아이템 장착
		EquipItem_Internal(ItemData, -1);
	}
}

void UEquipmentComponent::AttachItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData, AAuraCharacter* AuraCharacter, EItemSubGroup ItemGroup, FName SocketName)
{
	if (ItemData.ItemSubGroup == ItemGroup)
	{
		DetachItemMeshFromAuraCharacterMesh(ItemData.ItemSubGroup);
		
		UStaticMeshComponent* ItemMesh = NewObject<UStaticMeshComponent>(GetOwner());
		
		if (!ItemData.StaticMesh)
			return;

		ItemMesh->SetStaticMesh(ItemData.StaticMesh);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ItemMesh->SetCanEverAffectNavigation(false);
		ItemMesh->RegisterComponent();

		ItemMesh->AttachToComponent(
			AuraCharacter->GetMesh(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SocketName);
		
		AttachedMesh.Add(ItemGroup, ItemMesh);
	}
}

void UEquipmentComponent::AttachBootsItemMeshToAuraCharacterMesh_Internal(const FItemData& ItemData,
	AAuraCharacter* AuraCharacter)
{
	DetachItemMeshFromAuraCharacterMesh(ItemData.ItemSubGroup);
	
	UStaticMeshComponent* LeftItemMesh = NewObject<UStaticMeshComponent>(AuraCharacter);
	UStaticMeshComponent* RightItemMesh = NewObject<UStaticMeshComponent>(AuraCharacter);
	
	if (!ItemData.RightFootMesh || !ItemData.LeftFootMesh)
		return;
	
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
	
	AttachedMesh.Add(EItemSubGroup::Boots, RightItemMesh);
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
	
	if (auto Value = AttachedMesh.Find(ItemSubGroup))
	{
		if (auto Mesh = Value->Get())
		{
			if (Mesh->GetStaticMesh())
			{
				Mesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
				Mesh->DestroyComponent();
			}
		}
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
	
	auto* SlotEntry = GetSlotEntry(ItemSubGroup);
	if (!SlotEntry)
		return;
	
	FItemData CopiedItemData = ItemData;
	
	// 해당 슬롯에 기존에 장착된 아이템 장착 해제
	// Index == -1, 저장 데이터에서 가져옴(아이템 추가할 필요 없음)
	if (OriginIndex >= 0)
	{
		Server_UnequipItem(ItemSubGroup);
	}
	else 
	{
		ClearSlot(SlotEntry);
		SlotEntry->ItemData = CopiedItemData;
	}
		
	// 스텟 적용
	ApplyItemStat(CopiedItemData, SlotEntry);
	
	// 아이템이 어빌리티 태그들을 가지면 각 어빌리티를 ASC에 부여
	for (const auto TagAndLevel : CopiedItemData.AbilityTagAndLevel)
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
	for (const auto EffectAndStack : CopiedItemData.EffectAndStacks)
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
	for (const auto Pair : CopiedItemData.AbilityUpgradeTagAndLevel)
	{
		if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner()))
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
		InventoryComponent->RemoveItemToEquip(OriginIndex);
	
	SlotEntry->ItemData = CopiedItemData;
	SlotEntry->bIsSlotEquipped = true;
	GetSlots().MarkArrayDirty();
	
	// UI 반영
	uint8 ItemIndex = static_cast<uint8>(CopiedItemData.ItemSubGroup);
	OnEquipmentSlotChanged.Broadcast(ItemIndex);
	
	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner()))
	{
		if (AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(AuraPS->GetAbilitySystemComponent()->GetAvatarActor()))
		{
			// 아이템을 메시에 장착
			AttachItemMeshToAuraCharacterMesh(CopiedItemData, AuraCharacter);
		}
		// 툴팁 제거
		if (APlayerController* PlayerController = AuraPS->GetPlayerController())
		{
			UAuraAbilitySystemLibrary::GetOverlayWidgetController(PlayerController)->OnItemToolTipActivated.Broadcast(FItemData(), false);
		}
	}
		
	// 아이템 장착 델리게이트 호출
	OnItemEquipped.Broadcast(CopiedItemData);
}

void UEquipmentComponent::UnEquipItem_Internal(FEquipmentSlot* Slot)
{
	if (!AuraASC || !InventoryComponent || !Slot)
		return;
	
	if (Slot->ItemData.ItemGroup != EItemGroup::Equipment)
		return;
	
	EItemSubGroup SubGroup = Slot->ItemData.ItemSubGroup;
	auto ClearSlotTarget = GetSlotEntry(SubGroup);
	
	FItemData CopyedItemData = Slot->ItemData;
		
	ClearSlot(ClearSlotTarget);
		
	// 장착 해제된 아이템 인벤토리에 추가
	InventoryComponent->AddItem_Internal(CopyedItemData);
		
	// 이미 장착 중인 아이템 효과 해제
	EquipmentSlots.EquipmentSlotChanged(*Slot);
	EquipmentSlots.MarkItemDirty(*Slot);
}

void UEquipmentComponent::ApplyHPMPRatio(float NewHP, float NewMP)
{
	// AuraASC null 체크
	if (!AuraASC)
	{
		AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
	}

	// 그래도 null이면 적용 보류
	if (!AuraASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyHPMPRatio: AuraASC is null, skipping"));
		return;
	}
	
	if (auto AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		FGameplayEffectContextHandle HealHPHandleContext = AuraASC->MakeEffectContext();
		HealHPHandleContext.AddInstigator(GetOwner(), GetOwner());

		auto HealHPHandle = AuraASC->MakeOutgoingSpec(AuraGI->ItemApplyHPHealEffect, 1.f, HealHPHandleContext);
		if (HealHPHandle.IsValid())
		{
			HealHPHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Vital_Health, NewHP);
			AuraASC->ApplyGameplayEffectSpecToSelf(*HealHPHandle.Data);
		}
				
		FGameplayEffectContextHandle HealMPHandleContext = AuraASC->MakeEffectContext();
		HealMPHandleContext.AddInstigator(GetOwner(), GetOwner());

		auto HealMPHandle = AuraASC->MakeOutgoingSpec(AuraGI->ItemApplyMPHealEffect, 1.f, HealMPHandleContext);
		if (HealMPHandle.IsValid())
		{
			HealMPHandle.Data->SetSetByCallerMagnitude(FAuraGameplayTags::Get().Attributes_Vital_Mana, NewMP);
			AuraASC->ApplyGameplayEffectSpecToSelf(*HealMPHandle.Data);
		}
	}
}

void UEquipmentComponent::HUDInitialized()
{
	// HUD 가져오기
	// if (AAuraPlayerState* AuraPS = GetOwner<AAuraPlayerState>())
	// {
	// 	if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(AuraPS->GetPlayerController()))
	// 	{
	// 		if (AAuraHUD* AuraHUD = AuraPC->GetHUD<AAuraHUD>())
	// 		{
	// 			AuraHUD->InventoryMenuViewModel
	// 		}
	// 	}
	// }
	
	for (int32 i = 0; i < EquipmentSlots.Items.Num(); ++i)
	{
		OnEquipmentSlotChanged.Broadcast(i);
	}
	
	// 델리게이트 바인딩, 즉시 호출
}

void UEquipmentComponent::OnCharacterInitialized(ACharacter* AvatarCharacter)
{
	ReEquipItem();
}

void UEquipmentComponent::Server_EquipItem_Implementation(const FItemData& ItemData, int OriginIndex)
{
	EquipItem_Internal(ItemData, OriginIndex);
	
	for (FEquipmentSlot& Entry : EquipmentSlots.Items)
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
	auto EquipSlot = GetSlotEntry(Slot);
	if (EquipSlot && !EquipSlot->IsEmpty())
	{
		UnEquipItem_Internal(EquipSlot);
	}
}

