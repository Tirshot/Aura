// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/ItemToolTipWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AttributeInfo.h"
#include "UI/Widget/AuraToolTipStatRow.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"

FItemToolTipRowData::FItemToolTipRowData()
{
}

void UItemToolTipWidgetController::AddStatRow(const FGameplayTag& AttributeTag, const float Stat)
{
	// 값이 0이거나 0에 가까우면 리턴
	if (FMath::IsNearlyZero(Stat))
		return;
	
	// 태그가 배열의 태그와 일치할 경우(배열에 이미 존재하는 경우) 값을 반환
	if (FItemToolTipRowData* Found = StatRows.FindByPredicate([&](const FItemToolTipRowData& Row){return Row.Tag == AttributeTag;}))
	{
		// 이미 있으면 값 갱신
		Found->Value = Stat;
	}
	else
	{
		// 없으면 추가
		StatRows.Add(FItemToolTipRowData(AttributeTag, Stat));
	}
}

void UItemToolTipWidgetController::ClearStatRows()
{
	if (!ToolTipWidget)
		return;
	
	StatRows.Empty();
	ToolTipWidget->VerticalBox_ItemStat->ClearChildren();
}

void UItemToolTipWidgetController::ChangeTextColorByUpgradeTag(UAuraToolTipStatRow* StatRow, FString BaseString, FString& OutString)
{
	FGameplayTag DirectParentTag = StatRow->Tag.RequestDirectParent();
	if (DirectParentTag.MatchesTag(FGameplayTag::RequestGameplayTag("Upgrades.Fire")))
	{
		OutString = FString::Printf(TEXT("<Fire>%s</>"), *BaseString);
	}
	else if (DirectParentTag.MatchesTag(FGameplayTag::RequestGameplayTag("Upgrades.Lightning")))
	{
		OutString = FString::Printf(TEXT("<Lightning>%s</>"), *BaseString);
	}
	else if (DirectParentTag.MatchesTag(FGameplayTag::RequestGameplayTag("Upgrades.Arcane")))
	{
		OutString = FString::Printf(TEXT("<Arcane>%s</>"), *BaseString);
	}
	else
	{
		OutString = BaseString;
	}
}

void UItemToolTipWidgetController::ChangeTextColorByAbilityTag(UAuraToolTipStatRow* StatRow, FString BaseString, FString& OutString)
{
	FGameplayTag DirectParentTag = StatRow->Tag.RequestDirectParent();
	if (DirectParentTag.MatchesTag(FGameplayTag::RequestGameplayTag("Abilities.Fire")))
	{
		OutString = FString::Printf(TEXT("<Fire>%s</>"), *BaseString);
	}
	else if (DirectParentTag.MatchesTag(FGameplayTag::RequestGameplayTag("Abilities.Lightning")))
	{
		OutString = FString::Printf(TEXT("<Lightning>%s</>"), *BaseString);
	}
	else if (DirectParentTag.MatchesTag(FGameplayTag::RequestGameplayTag("Abilities.Arcane")))
	{
		OutString = FString::Printf(TEXT("<Arcane>%s</>"), *BaseString);
	}
	else
	{
		OutString = BaseString;
	}
}

void UItemToolTipWidgetController::SetItemDataToWidget(const FItemData& ItemData)
{
	const FAuraGameplayTags& AuraTags = FAuraGameplayTags::Get();
	
	// 아이템 이름, 설명 설정
	FString ItemName = ItemData.DisplayName.ToString();
	EUpgradeRarity ItemRarity = ItemData.Rarity;
	FString ItemNameWithTextStyle = FString();
	
	switch (ItemRarity)
	{
	case EUpgradeRarity::Common:
		ItemNameWithTextStyle = FString::Printf(TEXT("<Common>%s</>"), *ItemName);
		break;
		
	case EUpgradeRarity::Rare:
		ItemNameWithTextStyle = FString::Printf(TEXT("<Rare>%s</>"), *ItemName);
		break;
		
	case EUpgradeRarity::Unique:
		ItemNameWithTextStyle = FString::Printf(TEXT("<Unique>%s</>"), *ItemName);
		break;
		
	case EUpgradeRarity::Legendary:
		ItemNameWithTextStyle = FString::Printf(TEXT("<Legendary>%s</>"), *ItemName);
		break;
	}
	
	// 아이템 이름, 아이템 설명
	ToolTipWidget->RichTextBlock_ItemName->SetText(FText::FromString(ItemNameWithTextStyle));
	ToolTipWidget->RichTextBlock_ItemDesc->SetText(ItemData.Description);
	
	// 슬롯 칸 수
	FString SlotSize = FString();
	SlotSize = FString::Printf(TEXT("(%dx%d)"), ItemData.Size.X, ItemData.Size.Y);
	ToolTipWidget->RichTextBlock_SlotSize->SetText(FText::FromString(SlotSize));
	
	// 스탯 설명 설정
	AddStatRow(AuraTags.Attributes_Primary_Strength, ItemData.ItemStat.Strength);
	AddStatRow(AuraTags.Attributes_Primary_Intelligence, ItemData.ItemStat.Intelligence);
	AddStatRow(AuraTags.Attributes_Primary_Resilience, ItemData.ItemStat.Resilience);
	AddStatRow(AuraTags.Attributes_Primary_Vigor, ItemData.ItemStat.Vigor);
	AddStatRow(AuraTags.Attributes_Secondary_MovementSpeed, ItemData.ItemStat.MovementSpeed);
	AddStatRow(AuraTags.Attributes_Secondary_MaxHealth, ItemData.ItemStat.MaxHealth);
	AddStatRow(AuraTags.Attributes_Secondary_MaxMana, ItemData.ItemStat.MaxMana);
	AddStatRow(AuraTags.Attributes_Secondary_MagicAttackPower, ItemData.ItemStat.MagicAttackPower);
	AddStatRow(AuraTags.Attributes_Secondary_Armor, ItemData.ItemStat.Armor);
	AddStatRow(AuraTags.Attributes_Secondary_ArmorPenetration, ItemData.ItemStat.ArmorPenetration);
	AddStatRow(AuraTags.Attributes_Secondary_CriticalHitChance, ItemData.ItemStat.CriticalHitChance);
	AddStatRow(AuraTags.Attributes_Secondary_HealthRegeneration, ItemData.ItemStat.HealthRegeneration);
	AddStatRow(AuraTags.Attributes_Secondary_ManaRegeneration, ItemData.ItemStat.ManaRegeneration);
	
	// Row 위젯을 생성해서 추가
	for (const auto& StatRow : StatRows)
	{
		if (UAuraToolTipStatRow* StatRowWidget = CreateWidget<UAuraToolTipStatRow>(GetWorld(), StatRowClass))
		{
			if (UAttributeInfo* Info = UAuraAbilitySystemLibrary::GetAttributeMenuWidgetController(this)->GetAttributeInfo())
			{
				FText StatName = Info->GetAttributeNameForTag(StatRow.Tag);
				FText StatValue = FText::AsNumber(StatRow.Value);
				
				if (StatName.IsEmpty())
					continue;
			
				StatRowWidget->RichTextBlock_StatName->SetText(StatName);
				StatRowWidget->RichTextBlock_StatCounts->SetText(StatValue);
			
				ToolTipWidget->VerticalBox_ItemStat->AddChildToVerticalBox(StatRowWidget);
			}
		}
	}
	
	// 게임플레이 이펙트, 어빌리티 설명 설정

	// 게임플레이 이펙트
	// for (const auto Pair : ItemData.EffectAndStacks)
	// {
	// 	Pair.EffectClass;
	// 	Pair.EffectStack;
	// }
	
	// 어빌리티 데이터에 접근
	if (UAbilityInfo* Info = UAuraAbilitySystemLibrary::GetAbilityInfo(this))
	{
		// 게임플레이 어빌리티
		for (const auto Pair : ItemData.AbilityTagAndLevel)
		{
			const FGameplayTag& AbilityTag = Pair.AbilityTag;
			const float& AbilityLevel = Pair.AbilityLevel;
			
			if (UAuraToolTipStatRow* StatRow = CreateWidget<UAuraToolTipStatRow>(GetWorld(), StatRowClass))
			{
				FString AbilityNameBase = Info->GetAbilityNameForTag(AbilityTag).ToString();
				FString AbilityName = TEXT("");
				FString Plus = TEXT("+");
				FString Counts = TEXT("");
				StatRow->Tag = AbilityTag;
				
				ChangeTextColorByAbilityTag(StatRow, AbilityNameBase, AbilityName);
				ChangeTextColorByAbilityTag(StatRow, TEXT(" +"), Plus);
				ChangeTextColorByAbilityTag(StatRow, FString::FromInt(AbilityLevel), Counts);
				
				StatRow->RichTextBlock_StatName->SetText(FText::FromString(AbilityName));
				StatRow->RichTextBlock_Plus->SetText(FText::FromString(Plus));
				StatRow->RichTextBlock_StatCounts->SetText(FText::FromString(Counts));
				ToolTipWidget->VerticalBox_EffectAndAbility->AddChildToVerticalBox(StatRow);
			}
		}
	}

	// 스펠 업그레이드
	if (UAbilityUpgradeInfo* Info = UAuraAbilitySystemLibrary::GetAbilityUpgradeInfo(this))
	{
		// 게임플레이 어빌리티
		for (const auto Pair : ItemData.AbilityUpgradeTagAndLevel)
		{
			const FGameplayTag& UpgradeTag = Pair.AbilityTag;
			const float& UpgradeStack = Pair.AbilityLevel;
			
			if (UAuraToolTipStatRow* StatRow = CreateWidget<UAuraToolTipStatRow>(GetWorld(), StatRowClass))
			{
				FAuraAbilityUpgradeInfo UpgradeInfo = Info->GetUpgradeInfoForUpgradeTag(UpgradeTag);
				FString AbilityName = TEXT("");
				FString Plus = TEXT("+");
				FString Stacks = TEXT("");
				StatRow->Tag = UpgradeTag;
				
				// 태그에 따라 색상 변경
				ChangeTextColorByUpgradeTag(StatRow, UpgradeInfo.UpgradeName, AbilityName);
				ChangeTextColorByUpgradeTag(StatRow, TEXT(" +"), Plus);
				ChangeTextColorByUpgradeTag(StatRow, FString::FromInt(UpgradeStack), Stacks);
				
				StatRow->RichTextBlock_StatName->SetText(FText::FromString(AbilityName));
				StatRow->RichTextBlock_Plus->SetText(FText::FromString(Plus));
				StatRow->RichTextBlock_StatCounts->SetText(FText::FromString(Stacks));
				ToolTipWidget->VerticalBox_EffectAndAbility->AddChildToVerticalBox(StatRow);
			}
		}
	}
}
