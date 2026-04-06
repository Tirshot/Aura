// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/AuraUserWidget.h"
#include "GameOverWidget.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UGameOverWidget : public UAuraUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ReviveTimeOverride = 5.f;
};
