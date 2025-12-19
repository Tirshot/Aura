// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/VerticalBox.h"
#include "Interaction/MessageInterface.h"
#include "UI/Widget/AuraUserWidget.h"
#include "AuraMessageBoxWidget.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraMessageBoxWidget : public UAuraUserWidget, public IMessageInterface
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent)
	void AddTextMessageToBox(const FText& Message);
	
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UVerticalBox* VerticalBox_Root;
};
