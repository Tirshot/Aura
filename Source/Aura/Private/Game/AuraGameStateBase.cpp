// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameStateBase.h"

#include "Character/AuraBossMonster.h"
#include "Game/AuraGameModeBase.h"
#include "Player/AuraPlayerController.h"

void AAuraGameStateBase::AddPlayerToArray(AAuraPlayerController* AuraPC)
{
	Players.AddUnique(AuraPC);
}

void AAuraGameStateBase::OnBossMonsterDead(AActor* DeadActor)
{
	// 월드 상태 저장
	// GameAutoSave();
}

void AAuraGameStateBase::AddMonsterToArray(AAuraEnemy* Enemy)
{
	if (AAuraBossMonster* Boss = Cast<AAuraBossMonster>(Enemy))
	{
		BossCharacters.AddUnique(Boss);
		
		MultiCast_BossCharactersSpawned();
        
		if (!Boss->OnDeath.IsAlreadyBound(this, &AAuraGameStateBase::OnBossMonsterDead))
			Boss->OnDeath.AddDynamic(this, &AAuraGameStateBase::OnBossMonsterDead);
		
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
