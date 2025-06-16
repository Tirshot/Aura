// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SaveProgressWidgetController.h"

void USaveProgressWidgetController::BindCallbacksToDependencies()
{
	
}

void USaveProgressWidgetController::RemoveWidget()
{
	// 블루프린트에서 바인딩
	RemoveWidgetDelegate.Broadcast();
}
