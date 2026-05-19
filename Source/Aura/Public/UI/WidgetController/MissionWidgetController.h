// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "AbilitySystem/Data/MissionInfo.h"
#include "MissionWidgetController.generated.h"

struct FMissionData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_SevenParams(FOnObjectiveUpdate, FGameplayTag, MissionTag, FGameplayTag, ObjectiveTag, int32, ObjectiveIndex, FText, Description, float, CurrentValue, float, TargetValue, float, RemainingTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionWidgetRequiredSignature, const FMissionDataArray&, Missions);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAddMissionToWidgetSignature, const FMissionData&, Mission);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMissionFinishedToWidgetSignature, FGameplayTag, MissionTag, bool, bSuccess);

UCLASS(BlueprintType, Blueprintable)
class AURA_API UMissionWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	void TryBindToGameState();
	virtual void BindCallbacksToDependencies() override;
	
public:
	UFUNCTION()
	void OnMissionDataChanged(const FMissionDataArray& CurrentMissions);
	
	UFUNCTION()
	void OnMissionFinished(FGameplayTag MissionTag, bool bIsSucceed);
	
	FText GetFormattedDescription(const FMissionObjective& Objective);
	
public:
	UPROPERTY(BlueprintAssignable)
	FOnObjectiveUpdate OnObjectiveUpdateDelegate;
	
	UPROPERTY()
	FOnMissionWidgetRequiredSignature OnMissionWidgetRequired;
	
	UPROPERTY(BlueprintAssignable)
	FOnAddMissionToWidgetSignature OnAddMissionToWidget;
	
	UPROPERTY(BlueprintAssignable)
	FOnMissionFinishedToWidgetSignature OnMissionFinishedToWidget;
	
};
