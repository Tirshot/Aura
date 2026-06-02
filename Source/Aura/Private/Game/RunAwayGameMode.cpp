// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/RunAwayGameMode.h"

#include "Player/AuraPlayerController.h"

void ARunAwayGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	
	// 블록 태그 적용
	if (auto* AuraPC = Cast<AAuraPlayerController>(NewPlayer))
	{
		AuraPC->Server_ApplyInputBlockTag();
	}
}
