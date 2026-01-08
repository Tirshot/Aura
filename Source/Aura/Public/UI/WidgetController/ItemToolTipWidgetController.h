// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/Widget/ItemToolTipWidget.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "ItemToolTipWidgetController.generated.h"

struct FItemData;
class UAuraToolTipStatRow;

USTRUCT()
struct FItemToolTipRowData
{
	GENERATED_BODY()
	
	FItemToolTipRowData();
	FItemToolTipRowData(const FGameplayTag& InTag, const float& InValue) : Tag(InTag), Value(InValue) {}

	UPROPERTY()
	FGameplayTag Tag;
	
	UPROPERTY()
	float Value;
	
	bool operator==(const FItemToolTipRowData& Other) const
	{
		return Tag == Other.Tag;
	}
};

UCLASS(Blueprintable)
class AURA_API UItemToolTipWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()
	
public:
	void AddStatRow(const FGameplayTag& AttributeTag, const float Stat);
	
	UFUNCTION(BlueprintCallable)
	void ClearStatRows();
	void ChangeTextColorByUpgradeTag(UAuraToolTipStatRow* StatRow, FString BaseString, FString& OutString);
	void ChangeTextColorByAbilityTag(UAuraToolTipStatRow* StatRow, FString BaseString, FString& OutString);

	UFUNCTION(BlueprintCallable)
	void SetItemDataToWidget(const FItemData& ItemData);
	
	UFUNCTION(BlueprintCallable)
	void SetWidget(UItemToolTipWidget* Widget) {ToolTipWidget = Widget;}
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAuraToolTipStatRow> StatRowClass;
	
protected:
	UPROPERTY()
	TArray<FItemToolTipRowData> StatRows;
	
	UPROPERTY()
	TObjectPtr<UItemToolTipWidget> ToolTipWidget;
};
