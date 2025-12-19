// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraMessageBoxWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/MenuAnchor.h"
#include "Interaction/MessageInterface.h"
#include "UI/Widget/AuraUserWidget.h"
#include "AuraOverlayWidget.generated.h"

class UAuraCenterDescriptionWidget;
/**
 * 
 */
UCLASS()
class AURA_API UAuraOverlayWidget : public UAuraUserWidget, public IMessageInterface
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(BlueprintCallable)
	void ClearKeyboardFocus();
	
	UFUNCTION(BlueprintCallable)
	void RemoveCenterDescriptionText();
	
public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UAuraMessageBoxWidget* WBP_MessageBox;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UAuraCenterDescriptionWidget* WBP_CenterTutorialDescription;
	
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UCanvasPanel* CanvasPanel_CenterDescription;
};
