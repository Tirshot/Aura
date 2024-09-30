// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/AuraHUD.h"
#include "UI/Widget/AuraUserWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"

UOverlayWidgetController *AAuraHUD::GetOverlayWidgetController(const FWidgetControllerParams &WCParams)
{
    if (OverlayWidgetController == nullptr)
    {   // 없으면 생성
        OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
        OverlayWidgetController->SetWidgetControllerParams(WCParams);
        OverlayWidgetController->BindCallbacksToDependencies();
    }

    return OverlayWidgetController;
}

UAttributeMenuWidgetController* AAuraHUD::GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams)
{
    if (AttributeMenuWidgetController == nullptr)
    {   // 없으면 생성
        AttributeMenuWidgetController = NewObject<UAttributeMenuWidgetController>(this, OverlayWidgetControllerClass);
        AttributeMenuWidgetController->SetWidgetControllerParams(WCParams);
        AttributeMenuWidgetController->BindCallbacksToDependencies();
    }

    return AttributeMenuWidgetController;
}

void AAuraHUD::InitOverlay(APlayerController *PC, APlayerState *PS, UAbilitySystemComponent *ASC, UAttributeSet *AS)
{
    // 위젯과 위젯 컨트롤러 생성
    checkf(OverlayWidgetClass, TEXT("오버레이 위젯 클래스가 초기화되지 않음. BP_AuraHUD"));
    checkf(OverlayWidgetControllerClass, TEXT("오버레이 위젯 컨트롤러 클래스가 초기화되지 않음. BP_AuraHUD"));

    // 위젯 생성 후 오라 유저 위젯으로 캐스팅
    UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
    OverlayWidget = Cast<UAuraUserWidget>(Widget);

    // 구조체에 할당 후 오버레이 위젯 컨트롤러를 초기화
    const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
    UOverlayWidgetController* WidgetController = GetOverlayWidgetController(WidgetControllerParams);

    // 위젯에 위젯 컨트롤러를 연결
    OverlayWidget->SetWidgetController(WidgetController);
    
    // 유효한 속성 세트와 위젯 컨트롤러를 가짐 -> 값 초기화 가능
    WidgetController->BroadcastInitialValues();
    Widget->AddToViewport();
}