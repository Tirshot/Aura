// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "GameOverWidgetController.generated.h"

class UAuraUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRestartTimer, float, RemainingTime);

UCLASS(BlueprintType, Blueprintable)
class AURA_API UGameOverWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	UFUNCTION()
	void HandleOnDeath(AActor* DeadActor);

	void SetRemainingTime(float InRemainingTime);

	UPROPERTY(BlueprintAssignable)
	FRestartTimer RestartTimer;

	UFUNCTION(BlueprintCallable)
	void RestartGame();
	
private:
	UPROPERTY()
	TObjectPtr<UAuraUserWidget> GameOverWidget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAuraUserWidget> GameOverWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	float RemainingTime = 0.f;
};
