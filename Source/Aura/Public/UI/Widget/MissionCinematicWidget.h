// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/AuraUserWidget.h"
#include "MissionCinematicWidget.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMissionCinematicWidget : public UAuraUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(BlueprintReadWrite)
	FText MissionTitle;
	
	UPROPERTY(BlueprintReadWrite)
	FText MissionDescription;
};
