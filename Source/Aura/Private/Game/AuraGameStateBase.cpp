// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameStateBase.h"

#include "AbilitySystem/Data/MissionInfo.h"
#include "Character/AuraBossMonster.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Player/AuraPlayerController.h"

void AAuraGameStateBase::BeginPlay()
{
	Super::BeginPlay();
	
	CurrentMissions.OwningGameState = this;
}

void AAuraGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AAuraGameStateBase, CurrentMissions);
}

void AAuraGameStateBase::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);
	
	if (auto PC = PlayerState->GetPlayerController())
	{
		if (auto AuraPC = Cast<AAuraPlayerController>(PC))
		{
			AddPlayerToArray(AuraPC);
		}
	}
}

void AAuraGameStateBase::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);
	
	if (auto PC = PlayerState->GetPlayerController())
	{
		if (auto AuraPC = Cast<AAuraPlayerController>(PC))
		{
			RemovePlayerFromArray(AuraPC);
		}
	}
}

void AAuraGameStateBase::AddPlayerToArray(AAuraPlayerController* AuraPC)
{
	Players.AddUnique(AuraPC);
	OnPlayerCountChanged.Broadcast(Players.Num(), AuraPC);
}

void AAuraGameStateBase::RemovePlayerFromArray(AAuraPlayerController* AuraPC)
{
	Players.Remove(AuraPC);
	OnPlayerCountChanged.Broadcast(Players.Num(), AuraPC);
}

void AAuraGameStateBase::AddMonsterToArray(AAuraEnemy* Enemy)
{
	if (AAuraBossMonster* Boss = Cast<AAuraBossMonster>(Enemy))
	{
		BossCharacters.AddUnique(Boss);
		MultiCast_BossCharactersSpawned();
		OnBossMonsterCountChanged.Broadcast(BossCharacters.Num());
	}
	else
	{
		EnemyCharacters.Add(Enemy);
		OnMonsterCountChanged.Broadcast(EnemyCharacters.Num());
	}
}

void AAuraGameStateBase::RemoveMonsterFromArray(AAuraEnemy* Enemy)
{
	if (AAuraBossMonster* Boss = Cast<AAuraBossMonster>(Enemy))
	{
		MultiCast_OnBossMonsterDead(Enemy);
		BossCharacters.Remove(Boss);
		OnBossMonsterCountChanged.Broadcast(BossCharacters.Num());
	}
	else
	{
		EnemyCharacters.Remove(Enemy);
		OnMonsterCountChanged.Broadcast(EnemyCharacters.Num());
	}
}

void AAuraGameStateBase::MultiCast_BossCharactersSpawned_Implementation()
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(PC))
		AuraPC->BossMonsterBind();
}

void AAuraGameStateBase::MultiCast_OnBossMonsterDead_Implementation(AActor* DeadActor)
{
	// 모든 클라이언트 대상으로 줌인 호출
	for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!IsValid(PC))
			continue;
		
		if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(PC))
		{
			AuraPC->Client_OnBossDead(DeadActor);
		}
	}
}

void AAuraGameStateBase::UpdateMissionData(FMissionData& MissionData)
{
	// 서버에서만 실행됨
	if (!HasAuthority())
		return;
	
	auto& Missions = CurrentMissions.Missions;
	
	int32 Index = Missions.IndexOfByPredicate([&](const FMissionData& Data)
	{
		return Data.MissionTag == MissionData.MissionTag;
	});

	if (Index != INDEX_NONE)
	{
		if (MissionData.bIsEnded)
		{
			RemoveMissionData(MissionData.MissionTag);
			return;
		}
		// 미션 갱신
		Missions[Index] = MissionData;
		CurrentMissions.MarkItemDirty(Missions[Index]);
	}
	else
	{
		// 미션 추가
		FMissionData& NewItem = Missions.Add_GetRef(MissionData);
		CurrentMissions.MarkItemDirty(NewItem);
	}

	// 리슨 서버 UI 갱신
	BroadcastMissionData();
}

void AAuraGameStateBase::RemoveMissionData(const FGameplayTag& MissionTag)
{
	// 태그가 일치하는 미션 데이터를 찾아 삭제
	int32 RemovedCount = CurrentMissions.Missions.RemoveAll([&MissionTag](const FMissionData& Data)
	{
		return Data.MissionTag.MatchesTagExact(MissionTag);
	});

	if (RemovedCount > 0)
	{
		// 리슨 서버 UI 갱신
		CurrentMissions.MarkArrayDirty();
		BroadcastMissionData();
	}
}

void AAuraGameStateBase::BroadcastMissionData()
{
	OnMissionDataChanged.Broadcast(CurrentMissions);
}

void AAuraGameStateBase::Multicast_MissionFinished_Implementation(const FGameplayTag& MissionTag, bool bIsSucceed)
{
	OnMissionFinishedSignature.Broadcast(MissionTag, bIsSucceed);
}
