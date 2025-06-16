// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/GameOverWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Character/AuraCharacter.h"
#include "Game/AuraGameInstance.h"
#include "Game/AuraGameModeBase.h"
#include "Game/LoadScreenSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UI/Widget/AuraUserWidget.h"

void UGameOverWidgetController::BroadcastInitialValues()
{

}

void UGameOverWidgetController::BindCallbacksToDependencies()
{
	if (GetAuraASC())
	{
		if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAuraASC()->GetAvatarActor()))
		{
			FOnDeath& OnDeathDelegate = CombatInterface->GetOnDeathDelegate();
			OnDeathDelegate.AddDynamic(this, &UGameOverWidgetController::HandleOnDeath);
		}
	}
}

void UGameOverWidgetController::HandleOnDeath(AActor* DeadActor)
{
	// 게임오버 위젯 생성
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
	GameOverWidget = Cast<UAuraUserWidget>(Widget);

	// 위젯에 위젯 컨트롤러를 연결
	GameOverWidget->SetWidgetController(this);

	// 기능 실행 및 뷰포트에 추가
	BroadcastInitialValues();
	Widget->AddToViewport();
}

void UGameOverWidgetController::SetRemainingTime(float InRemainingTime)
{
	RemainingTime = InRemainingTime;

	// 타이머 시작
	RestartTimer.Broadcast(InRemainingTime);
}

void UGameOverWidgetController::RestartGame()
{
	AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
	if (AuraGM == nullptr)
		return;

	// UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(AuraGM->GetGameInstance());
	// if (AuraGameInstance == nullptr)
	// 	return;
	//
	// const FString InGameLoadSlotName = AuraGameInstance->LoadSlotName;
	// const int32 InGameLoadSlotIndex = AuraGameInstance->LoadSlotIndex;
	//
	// ULoadScreenSaveGame* SaveGame = AuraGM->GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
	// if (IsValid(SaveGame) == false)
	// 	return;
	//
	// UGameplayStatics::OpenLevel(GameOverWidget, )

	AuraGM->RestartGameFromSaveDataWithWorldContextObject(GameOverWidget);
}
