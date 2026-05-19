// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/MissionWidgetController.h"

#include "AbilitySystem/Data/MissionInfo.h"
#include "Game/AuraGameStateBase.h"

void UMissionWidgetController::BroadcastInitialValues()
{
	Super::BroadcastInitialValues();
	
	// 초기 진입 시점에 이미 진행 중인 미션이 있다면 UI 생성 요청
	if (AAuraGameStateBase* AuraGS = Cast<AAuraGameStateBase>(GetWorld()->GetGameState()))
	{
		OnMissionDataChanged(AuraGS->GetCurrentMissions());
		
		const FMissionDataArray& InitialMissions = AuraGS->GetCurrentMissions();

		// 데이터가 있다면 즉시 호출하여 초기 위젯 생성
		if (InitialMissions.Missions.Num() > 0)
		{
			OnMissionDataChanged(InitialMissions);
		}
	}
}

void UMissionWidgetController::TryBindToGameState()
{
	if (AAuraGameStateBase* AuraGS = GetWorld()->GetGameState<AAuraGameStateBase>())
	{
		AuraGS->OnMissionDataChanged.AddDynamic(this, &UMissionWidgetController::OnMissionDataChanged);
		AuraGS->OnMissionFinishedSignature.AddDynamic(this, &UMissionWidgetController::OnMissionFinished);
		
		BroadcastInitialValues();
	}
	else
	{
		// 다시 시도
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &UMissionWidgetController::TryBindToGameState, 0.1f, false);
	}
}

void UMissionWidgetController::BindCallbacksToDependencies()
{
	Super::BindCallbacksToDependencies();
	
	TryBindToGameState();	
}

FText UMissionWidgetController::GetFormattedDescription(const FMissionObjective& Objective)
{
	FText FormattedDesc = FText::Format(
			Objective.ObjectiveDescription,
			FFormatNamedArguments{{ TEXT("TargetValue"), FText::AsNumber(Objective.TargetValue)}}
		);

	// 필수 오브젝티브라면 [필수] 접두사를 추가
	if (Objective.bIsRequired)
	{
		return FText::Format(
			NSLOCTEXT("Mission", "RequiredPrefix", "[필수] {0}"),
			FormattedDesc
		);
	}

	return FormattedDesc;
}

void UMissionWidgetController::OnMissionDataChanged(const FMissionDataArray& CurrentMissions)
{
	// 위젯 생성 요청 델리게이트
	OnMissionWidgetRequired.Broadcast(CurrentMissions);
	
	for (const FMissionData& Data : CurrentMissions.Missions)
	{
		// 활성화된 미션이 아니면 스킵
		if (!Data.bIsActive)
			continue;

		for (const FMissionObjective& Objective : Data.Objectives)
		{
			FText FormattedDesc = GetFormattedDescription(Objective);

			// 2개 이상의 미션이 있을 때, 일치하는 미션 내에만 들어가도록 지정
			if (Objective.MyMissionTag.MatchesTagExact(Data.MissionTag))
			{
				int32 RoundCurrentValue = FMath::RoundToInt32(Objective.CurrentValue);
				
				// 위젯에 전송
				OnObjectiveUpdateDelegate.Broadcast(
					Data.MissionTag,
					Objective.ObjectiveTag,
					Objective.ObjectiveIndex,
					FormattedDesc, 
					RoundCurrentValue, 
					Objective.TargetValue, 
					Data.TimeRemaining
				);
			}
		}
	}
}

void UMissionWidgetController::OnMissionFinished(FGameplayTag MissionTag, bool bIsSucceed)
{
	OnMissionFinishedToWidget.Broadcast(MissionTag, bIsSucceed);
}
