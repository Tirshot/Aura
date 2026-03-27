// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CharmComponent.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemGlobals.h"
#include "Character/AuraCharacter.h"
#include "Interaction/PlayerInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"
#include "Player/InventoryComponent.h"

UCharmComponent::UCharmComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}


void UCharmComponent::TryToBindItemGetAndRemove()
{
	if (APawn* Pawn = Cast<AAuraPlayerState>(GetOwner())->GetPawn())
	{
		if (UInventoryComponent* Inventory = IPlayerInterface::Execute_GetInventoryComponent(Pawn))
		{
			if (!Inventory->OnItemGet.IsAlreadyBound(this, &UCharmComponent::AddToCharmSlot))
				Inventory->OnItemGet.AddDynamic(this, &UCharmComponent::AddToCharmSlot);
		
			if (!Inventory->OnItemRemoved.IsAlreadyBound(this, &UCharmComponent::RemoveFromCharmSlot))
				Inventory->OnItemRemoved.AddDynamic(this, &UCharmComponent::RemoveFromCharmSlot);
					
			if (!Inventory->SlotsReplicated.IsBoundToObject(this))
				Inventory->SlotsReplicated.AddUObject(this, &UCharmComponent::ApplyCharmEffectFromSavedInventory);
		}
	}
}

void UCharmComponent::ApplyCharmEffectFromSavedInventory()
{
	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner()))
	{
		// 인벤토리 순회
		APawn* Pawn = AuraPS->GetPawn();
		if (!Pawn)
			return;
		
		if (UInventoryComponent* Inventory = IPlayerInterface::Execute_GetInventoryComponent(Pawn))
		{
			for (const auto& Slot : Inventory->GetSlots())
			{
				if (!Slot.bIsOccupied)
					continue;
				
				if (Slot.ItemData.ItemGroup == EItemGroup::Charm)
				{
					// 아이템의 첫 슬롯에서만 효과 적용
					if (Inventory->IsFirstSlotOfItem(Slot))
					{
						AddToCharmSlot(Slot.SlotID, false);
					}
				}
			}
		}
	}
}

void UCharmComponent::BeginPlay()
{
	Super::BeginPlay();
	
	TryToBindItemGetAndRemove();
	
	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner()))
	{
		AuraPS->OnPawnSet.AddUniqueDynamic(this, &UCharmComponent::OnPawnSet);
	}
}

void UCharmComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UCharmComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UCharmComponent::RemoveCharmItemEffect(const FItemData& CharmItem)
{
	if (!CharmItem.Name.IsValid())
		return;
	
	if (CharmItem.ItemGroup != EItemGroup::Charm)
		return;
	
	auto ASC = UAuraAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC)
		return;

	auto AuraASC = Cast<UAuraAbilitySystemComponent>(ASC);
	if (!AuraASC)
		return;
	
	// 스텟 이펙트와 기타 부여된 게임플레이 이펙트 제거
	if (FCharmActiveEffects* ActiveEffects = AppliedCharms.Find(CharmItem.UniqueID))
	{
		for (auto ActiveEffectHandle : ActiveEffects->EffectHandles)
		{
			AuraASC->RemoveActiveGameplayEffect(ActiveEffectHandle, 1);
		}
	}
	
	// 맵에서 제거
	AppliedCharms.Remove(CharmItem.UniqueID);
	
	// 슬롯에 배치된 아이템에 의해 부여된 어빌리티 제거
	for (const auto TagAndLevel : CharmItem.AbilityTagAndLevel)
	{
		const FGameplayTag AbilityTag = TagAndLevel.AbilityTag;
		const int32 AbilityLevel = TagAndLevel.AbilityLevel;
		
		AuraASC->RemoveCharacterAbilityByTag(AbilityTag, AbilityLevel);
	}
}

void UCharmComponent::ApplyCharmItemEffect(const FItemData& CharmItem)
{
	auto ASC = UAuraAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!ASC)
		return;

	auto AuraASC = Cast<UAuraAbilitySystemComponent>(ASC);
	if (!AuraASC)
		return;
	
	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(AuraASC->GetAvatarActor());
	if (!AuraCharacter)
		return;
	
	FCharmActiveEffects& ActiveEffects = AppliedCharms.FindOrAdd(CharmItem.UniqueID);
	
	// 스텟 적용
	ApplyItemStat(CharmItem);
	
	// 아이템이 어빌리티 태그들을 가지면 각 어빌리티를 ASC에 부여
	for (const auto TagAndLevel : CharmItem.AbilityTagAndLevel)
	{
		const FGameplayTag AbilityTag = TagAndLevel.AbilityTag;
		const int32 AbilityLevel = TagAndLevel.AbilityLevel;
		
		for (int i = 0; i < AbilityLevel; i++)
		{
			AuraASC->AddCharacterAbilityByTag(AbilityTag);
		}
	}
	
	// 아이템이 게임플레이 이펙트 핸들을 가지면 이펙트를 ASC에 적용
	for (const auto EffectAndStack : CharmItem.EffectAndStacks)
	{
		if (EffectAndStack.EffectClass)
		{
			const FGameplayEffectContextHandle Context = AuraASC->MakeEffectContext();
			const auto SpecHandle = AuraASC->MakeOutgoingSpec(EffectAndStack.EffectClass, EffectAndStack.EffectStack, Context);
			FActiveGameplayEffectHandle ActiveHandle = AuraASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
			
			ActiveEffects.EffectHandles.Add(ActiveHandle);
		}
	}
	
	// 어빌리티 업그레이드 태그 추가
	// PlayerState에 접근 -> 어빌리티 업그레이드 태그 추가
	for (const auto Pair : CharmItem.AbilityUpgradeTagAndLevel)
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
}

void UCharmComponent::AddToCharmSlot(int SlotIndex, bool bIsItemMoved)
{
	// 아이템 추가가 인벤토리 내의 단순 이동이라면 리턴
	if (bIsItemMoved)
		return;
	
	if (UInventoryComponent* Inventory = IPlayerInterface::Execute_GetInventoryComponent(Cast<AAuraPlayerState>(GetOwner())->GetPawn()))
	{
		FInventorySlot* InventorySlot = Inventory->GetSlotByIndex(SlotIndex);
		if (!InventorySlot)
			return;
		
		FItemData& CharmData = InventorySlot->ItemData;
		if (CharmData.ItemGroup == EItemGroup::Charm)
		{
			// 이미 적용된 참은 무시
			if (AppliedCharms.Contains(CharmData.UniqueID))
				return;
			
			// 참 효과 적용
			ApplyCharmItemEffect(CharmData);
		}
	}
}

void UCharmComponent::RemoveFromCharmSlot(const FItemData& ItemData)
{
	if (GetOwnerRole() != ROLE_Authority)
		return;
	
	if (ItemData.ItemGroup == EItemGroup::Charm)
	{
		// 참 효과 제거
		RemoveCharmItemEffect(ItemData);
	}
}

void UCharmComponent::ApplyItemStat(const FItemData& ItemData)
{
	FItemStat ItemStat = ItemData.ItemStat;
	UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
	if (!AuraASC)
		return;
	
	FGameplayEffectContextHandle Context = AuraASC->MakeEffectContext();
	Context.AddInstigator(GetOwner(), GetOwner());
		
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
		
		FActiveGameplayEffectHandle ActiveHandle = AuraASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
		FCharmActiveEffects& ActiveEffects = AppliedCharms.FindOrAdd(ItemData.UniqueID);
		ActiveEffects.EffectHandles.Add(ActiveHandle);
	}
}

void UCharmComponent::OnPawnSet(APlayerState* PlayerState, APawn* NewPawn, APawn* OldPawn)
{
	if (!NewPawn)
		return;
	
	if (UInventoryComponent* Inventory = IPlayerInterface::Execute_GetInventoryComponent(NewPawn))
	{
		if (!Inventory->OnItemGet.IsAlreadyBound(this, &UCharmComponent::AddToCharmSlot))
			Inventory->OnItemGet.AddDynamic(this, &UCharmComponent::AddToCharmSlot);
		
		if (!Inventory->OnItemRemoved.IsAlreadyBound(this, &UCharmComponent::RemoveFromCharmSlot))
			Inventory->OnItemRemoved.AddDynamic(this, &UCharmComponent::RemoveFromCharmSlot);
		
		Inventory->SlotsReplicated.AddUObject(this, &UCharmComponent::ApplyCharmEffectFromSavedInventory);
		
		if (!Inventory->GetSlots().IsEmpty()) 
		{
			ApplyCharmEffectFromSavedInventory();
		}
	}
}
