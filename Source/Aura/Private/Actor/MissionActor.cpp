// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/MissionActor.h"

#include "LevelSequencePlayer.h"
#include "Character/AuraCharacter.h"
#include "Game/AuraGameModeBase.h"
#include "Game/AuraGameStateBase.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"
#include "Runtime/LevelSequence/Public/LevelSequenceActor.h"
#include "UI/HUD/AuraHUD.h"

AMissionActor::AMissionActor()
{
	PrimaryActorTick.bCanEverTick = true;

}

void AMissionActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		if (AAuraGameStateBase* GS = GetWorld()->GetGameState<AAuraGameStateBase>())
		{
			GS->OnPlayerCountChanged.AddDynamic(this, &AMissionActor::OnPlayerCountUpdated);
		}
	}
}

void AMissionActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMissionActor::StartMission()
{
	Multicast_PlayCinematic_Implementation();
	
	if (!HasAuthority())
		return;
	
	DynamicMissionData = MissionInfo->GetMissionData();
	DynamicMissionData.bIsActive = true;
	
	// 미션 목표 계수 설정, 미션 용 게임플레이 이펙트 적용
	if (AAuraGameStateBase* GS = GetWorld()->GetGameState<AAuraGameStateBase>())
	{
		CalculateMultiplier(GS->GetPlayersArray().Num());
		
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (!PC)
				continue;
			
			if (auto AuraPC = Cast<AAuraPlayerController>(PC))
			{
				ApplyMissionEffectToPC(AuraPC);
			}
		}
	}
	
	GetWorldTimerManager().SetTimer(MissionTimerHandle, this, &AMissionActor::UpdateTimer, 1.0f, true);
	
	// 수정된 데이터 동기화
	SyncDataToGameState();
	
	UE_LOG(LogTemp, Warning, TEXT("%s Mission Started!!"), *DynamicMissionData.MissionTag.ToString())
}

void AMissionActor::EndMission(bool bSucceed)
{
	if (!HasAuthority())
		return;
	
	if (!DynamicMissionData.bIsActive)
		return;
	
	DynamicMissionData.bIsActive = false;
	DynamicMissionData.bIsEnded = true;
		
	OnEndMissionDelegate.Broadcast();
	
	// 게임플레이 이펙트 제거
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
			continue;
		
		if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(PC))
		{
			AuraPC->GetASC()->RemoveActiveEffectsWithTags(ActiveEffectTags);
		}
	}
		
	// 타이머 핸들 제거
	GetWorldTimerManager().ClearTimer(MissionTimerHandle);
	
	bool bHasRequiredObjective = false; // 필수 목표가 존재하는가
	bool bAllRequiredReached = true;  // 모든 필수 목표 달성 여부
	bool bAnyOptionalReached = false; // 선택 목표 중 하나라도 달성했는가
	for (auto& Objective : DynamicMissionData.Objectives)
	{
		if (Objective.bIsRequired)
		{
			bHasRequiredObjective = true;
			if (!Objective.bIsReached)
			{
				bAllRequiredReached = false;
			}
		}
		else if (Objective.bIsReached)
		{
			bAnyOptionalReached = true;
		}
	}
	
	// 최종 성공 판정
	bool bIsMissionSuccess = false;
	
	// 성공 여부 판단
	if (bHasRequiredObjective)
	{
		// 필수 목표가 있다면 필수를 다 깨야 성공
		bIsMissionSuccess = bAllRequiredReached;
	}
	else
	{
		// 필수 목표가 하나도 없다면 -> 하나라도 깼거나 외부에서 bSucceed를 true로 줬을 때 성공
		bIsMissionSuccess = bAnyOptionalReached || bSucceed;
	}

	if (bIsMissionSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("Mission Accomplished!"));
        
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(PC->GetPawn()))
				{
					AuraCharacter->MulticastLevelUpParticles();
				}
			}
		}
		GiveRewards();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Mission Failed..."));
	}
		
	if (auto GameState = GetWorld()->GetGameState<AAuraGameStateBase>())
	{
		GameState->Multicast_MissionFinished(DynamicMissionData.MissionTag, bIsMissionSuccess);
	}
		
	SyncDataToGameState();
	GetWorldTimerManager().SetTimer(MissionTimerHandle, this, &AMissionActor::FinishAndDestroy, 5.0f, false);
	
	UE_LOG(LogTemp, Warning, TEXT("%s Mission End!!"), *DynamicMissionData.MissionTag.ToString())
}

void AMissionActor::Multicast_PlayCinematic_Implementation()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->IsLocalController())
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(PC->GetHUD()))
		{
			AuraHUD->HideOverlay();
			AuraHUD->CreateMissionCinematicWidget(MissionInfo->GetMissionData().MissionTitle, MissionInfo->GetMissionData().MissionDescription);
		}
	}
	
	if (IsValid(LevelSequenceActor))
	{
		// 시퀀스 종료 바인딩
		LevelSequenceActor->GetSequencePlayer()->OnFinished.AddDynamic(this, &AMissionActor::OnCinematicFinished);
		LevelSequenceActor->GetSequencePlayer()->Play();
	}
}

void AMissionActor::OnCinematicFinished()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC && PC->IsLocalController())
	{
		if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(PC->GetHUD()))
		{
			AuraHUD->ShowOverlay();
		}
	}
}

void AMissionActor::CalculateMultiplier(int32 PlayerCounts)
{
	// 플레이어 1명당 1.5배
	float Multiplier = 1.0f + FMath::Max(PlayerCounts - 1, 0) * 0.5f;

	for (int32 i = 0; i < DynamicMissionData.Objectives.Num(); ++i)
	{
		FMissionObjective& Objective = DynamicMissionData.Objectives[i];
		Objective.MyMissionTag = DynamicMissionData.MissionTag;
		Objective.ObjectiveIndex = i;
		
		// 요구량 고정 미션(몬스터 처치 미션, 트리거 미션 등)은 요구량을 증가시키지 않음
		if (DynamicMissionData.bSetValueFixed == true)
			continue;
		
		Objective.TargetValue *= Multiplier;
	}
}

void AMissionActor::ApplyMissionEffectToPC(AAuraPlayerController* JoinedPC)
{
	// 입장한 PC에게 게임플레이 이펙트 적용
	if (JoinedPC)
	{
		auto ASC = JoinedPC->GetASC();
		if (!ASC)
			return;

		for (const auto& EffectToApply: DynamicMissionData.EffectsToApplyOnStart)
		{
			// 이펙트 컨텍스트 핸들 생성
			FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
			EffectContextHandle.AddSourceObject(this);
				
			FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(EffectToApply, 1.f, EffectContextHandle);
			ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
				
			EffectSpecHandle.Data->GetAllAssetTags(ActiveEffectTags);
		}
	}
}

void AMissionActor::ReportValue(const FGameplayTag& ObjectiveTag, float Value)
{
	if (!HasAuthority() || !DynamicMissionData.bIsActive)
		return;
	
	bool bChanged = false;
	
	for (int32 i = 0; i < DynamicMissionData.Objectives.Num(); ++i)
	{
		auto& Objective = DynamicMissionData.Objectives[i];
		if (Objective.ObjectiveTag.MatchesTagExact(ObjectiveTag))
		{
			Objective.CurrentValue += Value;
			bChanged = true;
		}
	}

	if (bChanged)
	{
		// 목표 달성 여부
		CheckObjectiveComplete();
		
		// 데이터 동기화
		SyncDataToGameState();
	}
}

void AMissionActor::CheckObjectiveComplete()
{
	if (!HasAuthority())
		return;

	bool bChanged = false;

	for (auto& Objective : DynamicMissionData.Objectives)
	{
		// 아직 달성되지 않았고, 목표치에 도달했다면
		if (!Objective.bIsReached && Objective.CurrentValue >= Objective.TargetValue)
		{
			Objective.bIsReached = true;
            
			bChanged = true;

			UE_LOG(LogTemp, Log, TEXT("Mission Objective Reached!"));
		}
		
		if (Objective.bIsReached && Objective.CurrentValue < Objective.TargetValue)
		{
			Objective.bIsReached = false;
            
			bChanged = true;

			UE_LOG(LogTemp, Log, TEXT("Mission Objective Not Reached!"));
		}
	}

	// 변화가 있었을 때만 딱 한 번 동기화
	if (bChanged)
	{
		// 시간 제한 이전에 완료 시 미션을 종료할 것인가
		if (bEndMissionEarly)
		{
			bool bAllReached = true;
			for (const auto& Obj : DynamicMissionData.Objectives)
			{
				if (!Obj.bIsReached)
				{
					bAllReached = false;
					break;
				}
			}
			
			SyncDataToGameState();

			if (bAllReached)
			{
				EndMission(true);
			}
		}
	}
}

void AMissionActor::GiveRewards()
{
	if (!HasAuthority() || !MissionInfo)
		return;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
			continue;
		
		AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(PC);

		auto AuraPlayerState = PC->GetPlayerState<AAuraPlayerState>();
		if (!AuraPlayerState)
			continue;
		
		auto AuraGameMode = GetWorld()->GetAuthGameMode<AAuraGameModeBase>();
		if (!AuraGameMode)
			continue;
		
		for (const auto& Objective : DynamicMissionData.Objectives)
		{
			if (Objective.bIsReached)
			{
				for (const FMissionReward& Reward : Objective.Rewards)
				{
					switch (Reward.RewardType)
					{
					case EMissionRewardType::SpellPoint:
						AuraPlayerState->AddToSpellPoints(Reward.Amount);
						break;
				
					case EMissionRewardType::AttributePoint:
						AuraPlayerState->AddToAttributePoints(Reward.Amount);
						break;
				
					case EMissionRewardType::Item:
						//
						break;
				
					case EMissionRewardType::AbilityUpgrade:
						AuraPC->Server_CreateCardSelection(AuraPC->GetPawn());
						break;
				
					case EMissionRewardType::None:
						break;
				
					}
				}
			}
		}
	}
}

void AMissionActor::FinishAndDestroy()
{
	if (HasAuthority())
	{
		if (auto GameState = GetWorld()->GetGameState<AAuraGameStateBase>())
		{
			GameState->RemoveMissionData(DynamicMissionData.MissionTag);
		}
	}
	
	GetWorldTimerManager().ClearTimer(MissionTimerHandle);
	Destroy();
}

void AMissionActor::OnPlayerCountUpdated(int32 PlayerCounts, AAuraPlayerController* JoinedPC)
{
	CalculateMultiplier(PlayerCounts);
	
	ApplyMissionEffectToPC(JoinedPC);
}

void AMissionActor::UpdateTimer()
{
	if (!HasAuthority())
		return;

	// 시간 감소
	DynamicMissionData.TimeRemaining -= 1.0f;

	if (DynamicMissionData.TimeRemaining <= 0.f)
	{
		// 타이머 중지 및 미션 종료
		GetWorldTimerManager().ClearTimer(MissionTimerHandle);
		
		bool bMissionSuccess = true;
		for (const auto& Obj : DynamicMissionData.Objectives)
		{
			// 필수 목표인데 달성하지 못했다면 실패
			if (Obj.bIsRequired && !Obj.bIsReached)
			{
				bMissionSuccess = false;
				break;
			}
		}
		
		EndMission(bMissionSuccess);
		return;
	}

	// 복제
	SyncDataToGameState();
}

void AMissionActor::SyncDataToGameState()
{
	if (auto GameState = GetWorld()->GetGameState<AAuraGameStateBase>())
	{
		GameState->UpdateMissionData(DynamicMissionData);
	}
}

