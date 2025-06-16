// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/LoadScreenHUD.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModel/MVVM_LoadScreen.h"
#include "UI/Widget/LoadScreenWidget.h"

void ALoadScreenHUD::BeginPlay()
{
	Super::BeginPlay();

	// �ε� �޴��� �� �� ����
	LoadScreenViewModel = NewObject<UMVVM_LoadScreen>(this, LoadScreenViewModelClass);

	// �� �� �ʱ�ȭ - �ε� ������ �� �� ����
	LoadScreenViewModel->InitializeLoadSlots();

	// �ε� �޴��� �� ����
	LoadScreenWidget = CreateWidget<ULoadScreenWidget>(GetWorld(), LoadScreenWidgetClass);
	LoadScreenWidget->AddToViewport();

	// �ε� �޴��� �並 ��������Ʈ���� �ʱ�ȭ ��Ŵ
	LoadScreenWidget->BlueprintInitializeWidget();

	// ������ �ҷ�����
	LoadScreenViewModel->LoadData();
}
