// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/AuraGameModeBase.h"
#include "RunAwayGameMode.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API ARunAwayGameMode : public AAuraGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual void RestartPlayer(AController* NewPlayer) override;
};
