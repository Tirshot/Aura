// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/GameOverWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Player/AuraPlayerController.h"
#include "UI/Widget/AuraUserWidget.h"
#include "UI/Widget/GameOverWidget.h"

void UGameOverWidgetController::BroadcastInitialValues()
{
	// 부활 시간을 블루프린트의 값에서 가져옴
	if (GameOverWidget)
		ReviveTime = GameOverWidget->ReviveTimeOverride;
	
	RemainingTime = ReviveTime;
}

void UGameOverWidgetController::HandleOnDeath(AActor* DeadActor)
{
	// 게임오버 위젯 생성
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), GameOverWidgetClass);
	GameOverWidget = Cast<UGameOverWidget>(Widget);

	// 위젯에 위젯 컨트롤러를 연결
	GameOverWidget->SetWidgetController(this);

	// 기능 실행 및 뷰포트에 추가
	BroadcastInitialValues();
	Widget->AddToViewport();
	
	// 부활 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(RestartTimer, this, &UGameOverWidgetController::TimerStart, 1.0f, true);
}

void UGameOverWidgetController::TimerStart()
{
	RemainingTime -= 1.0f;
	if (RemainingTime <= 0.f)
	{
		ReviveFromRecentPlayerStart();
		GetWorld()->GetTimerManager().ClearTimer(RestartTimer);
		RemainingTime = ReviveTime;
		
		GameOverWidget->RemoveFromParent();
	}
}

void UGameOverWidgetController::ReviveFromRecentPlayerStart()
{
	// 사망자의 플레이어 컨트롤러의 서버 RPC 호출 델리게이트 발사
	if (IsValid(GetAuraPC()))
	{
		GetAuraPC()->Server_ReviveFromPlayerStart();
	}
}

void UGameOverWidgetController::TravelToMenu()
{
	// 세션에서 나가고 메뉴로 이동
	if (PlayerController)
	{
		PlayerController->ClientTravel("Game/Maps/LoadMenu", ETravelType::TRAVEL_Absolute);
	}
}
