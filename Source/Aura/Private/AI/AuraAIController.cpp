// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AuraAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Character/AuraCharacterBase.h"

AAuraAIController::AAuraAIController()
{
	Blackboard = CreateDefaultSubobject<UBlackboardComponent>("BlackboardComponent");
	check(Blackboard);

	BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>("BehaviorTreeComponent");
	check(BehaviorTreeComponent);
}

void AAuraAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

}

void AAuraAIController::Server_CharacterInvincible_Implementation(bool bInvincible)
{
	if (!HasAuthority())
		return;

	if (AAuraCharacterBase* AuraCharacterBase = GetPawn<AAuraCharacterBase>())
	{
		if (AuraCharacterBase->Implements<UCombatInterface>())
		{
			ICombatInterface::Execute_SetCharacterInvincible(AuraCharacterBase, bInvincible);
		}
	}
}
