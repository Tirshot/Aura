// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/LoadScreenWidget.h"

void ULoadScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void ULoadScreenWidget::SetViewModelInternal(UMVVMViewModelBase* NewViewModel)
{
	// 새 뷰 모델이 현재 뷰 모델과 다르다면 업데이트
	if (ViewModel != NewViewModel)
	{
		ViewModel = NewViewModel;

		if (ViewModel)
		{
			OnViewModelBound.AddDynamic(this, &ULoadScreenWidget::ViewModelBoundEvent);
			OnViewModelBound.Broadcast(ViewModel);
		}
	}
}
