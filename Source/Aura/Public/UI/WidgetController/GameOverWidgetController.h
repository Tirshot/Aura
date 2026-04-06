// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "GameOverWidgetController.generated.h"

class UGameOverWidget;
class UAuraUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRestartTimer, float, RemainingTime);

UCLASS(BlueprintType, Blueprintable)
class AURA_API UGameOverWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BroadcastInitialValues() override;

	UFUNCTION()
	void HandleOnDeath(AActor* DeadActor);
	
	UFUNCTION()
	void TimerStart();

	UFUNCTION(BlueprintCallable)
	void ReviveFromRecentPlayerStart();
	
	UFUNCTION(BlueprintCallable)
	void TravelToMenu();
	
public:
	UPROPERTY(BlueprintReadOnly)
	FTimerHandle RestartTimer;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float ReviveTime = 5.f;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float RemainingTime = 0.f;
	
protected:
	UPROPERTY()
	TObjectPtr<UGameOverWidget> GameOverWidget;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UAuraUserWidget> GameOverWidgetClass;
};
