// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/LoadScreenHUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModel/MVVM_LoadScreen.h"
#include "UI/Widget/LoadScreenWidget.h"
#include "UI/WidgetController/SettingsMenuWidgetController.h"

USettingsMenuWidgetController* ALoadScreenHUD::GetSettingsMenuWidgetController()
{
	if (SettingsMenuWidgetController == nullptr)
	{   // 없으면 생성
		SettingsMenuWidgetController = NewObject<USettingsMenuWidgetController>(this, SettingsMenuWidgetControllerClass);
		SettingsMenuWidgetController->BindCallbacksToDependencies();
	}
	
	return SettingsMenuWidgetController;
}

void ALoadScreenHUD::BeginPlay()
{
	Super::BeginPlay();

	// 로드 슬롯 위젯과 뷰 모델 초기화
	LoadScreenViewModel = NewObject<UMVVM_LoadScreen>(this, LoadScreenViewModelClass);
	LoadScreenViewModel->InitializeLoadSlots();
	
	LoadScreenWidget = CreateWidget<ULoadScreenWidget>(GetWorld(), LoadScreenWidgetClass);
	LoadScreenWidget->AddToViewport();
	LoadScreenWidget->BlueprintInitializeWidget();

	LoadScreenViewModel->LoadData();
	
	// 환경설정 메뉴 위젯 컨트롤러 생성
	SettingsMenuWidgetController = NewObject<USettingsMenuWidgetController>(this, SettingsMenuWidgetControllerClass);
	SettingsMenuWidgetController->Initialize();
}
