// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlock.h"
#include "UI/Widget/AuraUserWidget.h"
#include "GameplayTagContainer.h"
#include "AuraToolTipStatRow.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraToolTipStatRow : public UAuraUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	URichTextBlock* RichTextBlock_StatName;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	URichTextBlock* RichTextBlock_Plus;
	
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	URichTextBlock* RichTextBlock_StatCounts;
	
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag Tag;
};
