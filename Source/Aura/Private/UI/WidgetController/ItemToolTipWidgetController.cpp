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

void UItemToolTipWidgetController::SetItemDataToWidget(const FItemData& ItemData)
{
	const FAuraGameplayTags& AuraTags = FAuraGameplayTags::Get();
	
	// 아이템 이름, 설명 설정
	ToolTipWidget->RichTextBlock_ItemName->SetText(ItemData.DisplayName);
	ToolTipWidget->RichTextBlock_ItemDesc->SetText(ItemData.Description);
	
	// 스탯 설명 설정
	AddStatRow(AuraTags.Attributes_Primary_Strength, ItemData.ItemStat.Strength);
	AddStatRow(AuraTags.Attributes_Primary_Intelligence, ItemData.ItemStat.Intelligence);
	AddStatRow(AuraTags.Attributes_Primary_Resilience, ItemData.ItemStat.Resilience);
	AddStatRow(AuraTags.Attributes_Primary_Vigor, ItemData.ItemStat.Vigor);
	AddStatRow(AuraTags.Attributes_Secondary_MovementSpeed, ItemData.ItemStat.MovementSpeed);
	AddStatRow(AuraTags.Attributes_Secondary_MaxHealth, ItemData.ItemStat.MaxHealth);
	AddStatRow(AuraTags.Attributes_Secondary_MaxMana, ItemData.ItemStat.MaxMana);
	
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
			
				StatRowWidget->TextBlock_StatName->SetText(StatName);
				StatRowWidget->TextBlock_StatCounts->SetText(StatValue);
			
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
				FText AbilityName = FText::FromName(Info->GetAbilityNameForTag(AbilityTag));
				
				StatRow->TextBlock_StatName->SetText(AbilityName);
				StatRow->TextBlock_StatCounts->SetText(FText::AsNumber(AbilityLevel));
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
				FText AbilityName = FText::FromString(UpgradeInfo.UpgradeName);
				
				StatRow->TextBlock_StatName->SetText(AbilityName);
				StatRow->TextBlock_StatCounts->SetText(FText::AsNumber(UpgradeStack));
				ToolTipWidget->VerticalBox_EffectAndAbility->AddChildToVerticalBox(StatRow);
			}
		}
	}
}
