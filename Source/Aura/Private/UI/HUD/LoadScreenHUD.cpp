// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/LoadScreenHUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModel/MVVM_LoadScreen.h"
#include "UI/Widget/LoadScreenWidget.h"

void ALoadScreenHUD::BeginPlay()
{
	Super::BeginPlay();

	// 로드 메뉴의 뷰 모델 생성
	LoadScreenViewModel = NewObject<UMVVM_LoadScreen>(this, LoadScreenViewModelClass);

	// 뷰 모델 초기화 - 로드 슬롯의 뷰 모델 생성
	LoadScreenViewModel->InitializeLoadSlots();

	// 로드 메뉴의 뷰 생성
	LoadScreenWidget = CreateWidget<ULoadScreenWidget>(GetWorld(), LoadScreenWidgetClass);
	LoadScreenWidget->AddToViewport();

	// 로드 메뉴의 뷰를 블루프린트에서 초기화 시킴
	LoadScreenWidget->BlueprintInitializeWidget();

	// 데이터 불러오기
	LoadScreenViewModel->LoadData();
}
