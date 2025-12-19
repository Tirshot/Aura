// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TextBlock.h"
#include "UI/Widget/AuraUserWidget.h"
#include "AuraCenterDescriptionWidget.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraCenterDescriptionWidget : public UAuraUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock* TextBlock;
};
