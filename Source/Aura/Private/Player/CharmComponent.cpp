// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CharmComponent.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemGlobals.h"
#include "Character/AuraCharacter.h"
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerState.h"
#include "Player/InventoryComponent.h"

UCharmComponent::UCharmComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}


void UCharmComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (APawn* Pawn = Cast<AAuraPlayerState>(GetOwner())->GetPawn())
	{
		if (UInventoryComponent* Inventory = IPlayerInterface::Execute_GetInventoryComponent(Pawn))
		{
			if (!Inventory->OnItemGet.IsAlreadyBound(this, &UCharmComponent::AddToCharmSlot))
				Inventory->OnItemGet.AddDynamic(this, &UCharmComponent::AddToCharmSlot);
		
			if (!Inventory->OnItemRemoved.IsAlreadyBound(this, &UCharmComponent::RemoveFromCharmSlot))
				Inventory->OnItemRemoved.AddDynamic(this, &UCharmComponent::RemoveFromCharmSlot);
		}
	}
}

void UCharmComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
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
	
	// 슬롯에 배치된 아이템에 의해 부여된 효과 해제
	AuraASC->RemoveActiveGameplayEffectBySourceEffect(CharmItem.ItemStatEffectClass, nullptr, 1);
	
	// 게임플레이 이펙트 제거
	for (const auto EffectAndStack : CharmItem.EffectAndStacks)
	{
		AuraASC->RemoveActiveGameplayEffectBySourceEffect(EffectAndStack.EffectClass, nullptr, 1);
	}
	
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
	
	// 참 효과 재적용
	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(AuraASC->GetAvatarActor());
	if (!AuraCharacter)
		return;
	
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
			AuraASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
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
	if (GetOwnerRole() != ROLE_Authority)
		return;
	
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
			// 참 효과 적용
			ApplyCharmItemEffect(CharmData);
		}
	}
}

void UCharmComponent::RemoveFromCharmSlot(const FItemData& ItemData)
{
	// if (ItemData.ItemGroup == EItemGroup::Charm)
	// {
	// 	for (int RemoveIndex = CharmSlotArray.Num() - 1; RemoveIndex >= 0; RemoveIndex--)
	// 	{
	// 		if (CharmSlotArray[RemoveIndex] == ItemData)
	// 		{
	// 			CharmSlotArray.RemoveAt(RemoveIndex);
	// 			break;
	// 		}
	// 	}
	// }
	
	// 참 효과 제거
	RemoveCharmItemEffect(ItemData);
}

void UCharmComponent::ApplyItemStat(const FItemData& ItemData)
{
	FItemStat ItemStat = ItemData.ItemStat;
	UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()));
	if (!AuraASC)
		return;
	
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
	}
}
