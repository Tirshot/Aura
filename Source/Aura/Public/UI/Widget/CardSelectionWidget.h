// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/AuraUserWidget.h"
#include "CardSelectionWidget.generated.h"

class UMenuAnchor;

UCLASS()
class AURA_API UCardSelectionWidget : public UAuraUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UMenuAnchor* MenuAnchor_AreYouSure;
};
