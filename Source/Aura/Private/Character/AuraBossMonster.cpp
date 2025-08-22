// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraBossMonster.h"

#include "EngineUtils.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AI/AuraAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerController.h"

AAuraBossMonster::AAuraBossMonster()
{
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->bDoCollisionTest = false;
	SpringArm->SetRelativeRotation(FRotator(-30.f, -180.f, 0.f));
	SpringArm->TargetArmLength = 600.f;
	SpringArm->SetupAttachment(RootComponent);

	DeathCamera = CreateDefaultSubobject<UCameraComponent>("Camera");
	DeathCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	DeathCamera->bUsePawnControlRotation = false;
	DeathCamera->bAutoActivate = true;
}

void AAuraBossMonster::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
	
	// 보스 몬스터는 Event.Montage.Boss.RoarEnd 태그 이후에 AI가 작동
	// 클라이언트는 복제로 제공받음
	if (!HasAuthority())
		return;
	
	auto* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag("Event.Montage.Boss.RoarStart"))
		.AddUObject(this, &AAuraBossMonster::OnRoarStart);
		
		ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag("Event.Montage.Boss.RoarEnd"))
		.AddUObject(this, &AAuraBossMonster::OnRoarEnd);
	}
}

void AAuraBossMonster::Die(const FVector& DeathImpulse)
{
	// 랙돌 효과와 무기 드랍
	Super::Die(DeathImpulse);
}

void AAuraBossMonster::SetIsBeingShocked_Implementation(bool bInShock)
{
	bIsBeingShock = false;
}

void AAuraBossMonster::ChangeGlobalTimeDilationToDefault()
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
}

void AAuraBossMonster::HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
}

void AAuraBossMonster::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
}

void AAuraBossMonster::BeingShockedTagChanged()
{
}

void AAuraBossMonster::OnRoarStart(const FGameplayEventData* EventData)
{
	// 플레이어 컨트롤러에서 바인딩한 OnBossEventStart 델리게이트 호출
	OnBossEventStart.Broadcast(this);
}

void AAuraBossMonster::OnRoarEnd(const FGameplayEventData* EventData)
{
	// 플레이어 컨트롤러에서 바인딩한 OnBossEventEnd 델리게이트 호출
	OnBossEventEnd.Broadcast(this);
	
	// 비헤이비어 트리 작동
	AuraAIController->RunBehaviorTree(BehaviorTree);
	AuraAIController->GetBlackboardComponent()->SetValueAsBool("RoarEnd", true);
}
