// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/GameOverWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Player/AuraPlayerController.h"
#include "UI/Widget/AuraUserWidget.h"

void UGameOverWidgetController::BroadcastInitialValues()
{
	RemainingTime = ReviveTime;
}

void UGameOverWidgetController::BindCallbacksToDependencies()
{
	RemainingTime = ReviveTime;
	// if (GetAuraASC())
	// {
	// 	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAuraASC()->GetAvatarActor()))
	// 	{
	// 		FOnDeath* OnDeath = CombatInterface->GetOnDeathDelegate();
	// 		OnDeath.RemoveDynamic(this, &UGameOverWidgetController::HandleOnDeath);
	// 		OnDeath.AddDynamic(this, &UGameOverWidgetController::HandleOnDeath);
	// 	}
	// }
}

void UGameOverWidgetController::BeginDestroy()
{
	// if (GetAuraASC())
	// {
	// 	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAuraASC()->GetAvatarActor()))
	// 	{
	// 		FOnDeath* OnDeath = CombatInterface->GetOnDeathDelegate();
	// 		OnDeath.RemoveDynamic(this, &UGameOverWidgetController::HandleOnDeath);
	// 	}
	// }
	
	Super::BeginDestroy();
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
