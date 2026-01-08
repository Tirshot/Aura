// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/Widget/AuraUserWidget.h"
#include "ItemToolTipWidget.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UItemToolTipWidget : public UAuraUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta=(BindWidget))
	URichTextBlock* RichTextBlock_ItemName;
	
	UPROPERTY(meta=(BindWidget))
	URichTextBlock* RichTextBlock_SlotSize;
	
	UPROPERTY(meta=(BindWidget))
	UVerticalBox* VerticalBox_ItemStat;
		
	UPROPERTY(meta=(BindWidget))
	UVerticalBox* VerticalBox_EffectAndAbility;
	
	UPROPERTY(meta=(BindWidget))
	URichTextBlock* RichTextBlock_ItemDesc;
};
