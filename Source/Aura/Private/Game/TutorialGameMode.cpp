// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/TutorialGameMode.h"

#include "Engine/StreamableManager.h"

void ATutorialGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 튜토리얼 데이터 비동기 로드
	FStreamableManager StreamableManager;
	StreamableManager.RequestAsyncLoad(TutorialDataAsset, FStreamableDelegate::CreateUObject(this, &ATutorialGameMode::OnTutorialDataLoaded));

}

void ATutorialGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	bIsPlayerLoggedIn = true;

	if (TutorialDataAsset)
	{
		bTutorialDataSet = true;
		TryStartTutorial();
	}
}

void ATutorialGameMode::TryStartTutorial()
{
	TutorialStepChangedDelegate.Broadcast(TutorialDataAsset->GetTutorialDialogueEntryByIndex(0));
}

void ATutorialGameMode::OnTutorialDataLoaded()
{
	bTutorialDataSet = true;

	TryStartTutorial();
}
