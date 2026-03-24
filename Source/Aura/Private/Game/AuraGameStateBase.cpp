// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameStateBase.h"

#include "Character/AuraBossMonster.h"
#include "Game/AuraGameModeBase.h"
#include "Player/AuraPlayerController.h"

void AAuraGameStateBase::AddPlayerToArray(AAuraPlayerController* AuraPC)
{
	Players.AddUnique(AuraPC);
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