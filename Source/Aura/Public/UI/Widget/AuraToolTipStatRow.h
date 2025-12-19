// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "UI/Widget/AuraUserWidget.h"
#include "AuraToolTipStatRow.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraToolTipStatRow : public UAuraUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta=(BindWidget))
	UTextBlock* TextBlock_StatName;
	
	UPROPERTY(meta=(BindWidget))
	UTextBlock* TextBlock_StatCounts;
};
