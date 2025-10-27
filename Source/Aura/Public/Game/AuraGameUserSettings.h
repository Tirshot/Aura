// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "AuraGameUserSettings.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UAuraGameUserSettings* GetAuraGameUserSettings();
};
