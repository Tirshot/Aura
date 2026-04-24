// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModel/MVVM_AbilityCard.h"
#include "LoadScreenWidget.generated.h"

class UMenuAnchor;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnViewModelBound, UMVVMViewModelBase*, BoundViewModel);

UCLASS()
class AURA_API ULoadScreenWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void BlueprintInitializeWidget();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void ViewModelBoundEvent(UMVVMViewModelBase* ViewModelBase);
	
public:
	UPROPERTY(BlueprintReadWrite, Category = "MVVM", meta=(ExposeOnSpawn="true"), Setter="SetViewModelInternal")
	TObjectPtr<UMVVMViewModelBase> ViewModel;

	// 블루프린트 이벤트
	UPROPERTY(BlueprintAssignable, Category = "MVVM|Events")
	FOnViewModelBound OnViewModelBound;

protected:
	virtual void NativeConstruct() override;

private:
	// ViewModel에 새로운 값이 할당될 때마다 호출됨
	UFUNCTION()
	void SetViewModelInternal(UMVVMViewModelBase* NewViewModel);
};
