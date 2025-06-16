// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "SaveProgressWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRemoveSaveProgressWidget);
UCLASS(BlueprintType, Blueprintable)
class AURA_API USaveProgressWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BindCallbacksToDependencies() override;
	
	void RemoveWidget();
	
public:
	UPROPERTY(BlueprintAssignable, Category="WidgetControl")
	FRemoveSaveProgressWidget RemoveWidgetDelegate;
};
