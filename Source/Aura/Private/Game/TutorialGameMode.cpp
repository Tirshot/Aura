// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/TutorialGameMode.h"

#include "Character/AuraCharacter.h"
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
	
	// 튜토리얼에 사용될 델리게이트 바인딩
	// 아이템 장착
	if (APawn* PlayerPawn = NewPlayer->GetPawn())
	{
		if (AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(PlayerPawn))
		{
			if (UEquipmentComponent* EquipmentComponent = IPlayerInterface::Execute_GetEquipmentComponent(AuraCharacter))
			{
				EquipmentComponent->OnItemEquipped.AddDynamic(this, &ATutorialGameMode::OnItemEquipTutorialFinished);
			}
		}
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

void ATutorialGameMode::OnItemEquipTutorialFinished(const FItemData& EquippedItem)
{
	if (EquippedItem.ItemSubGroup == EItemSubGroup::Weapon)
	{
		TaskCompletedDelegate.Broadcast();
	}
}
