// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraBossMonster.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AI/AuraAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Camera/CameraComponent.h"
#include "Game/AuraGameModeBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"

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

	// 몽타주 태그와 콜백 바인딩
	auto* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag("Event.Montage.Boss.RoarStart"))
		.AddUObject(this, &AAuraBossMonster::OnRoarStart);
		
		ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag("Event.Montage.Boss.RoarEnd"))
		.AddUObject(this, &AAuraBossMonster::OnRoarEnd);
	}

	// 광폭화 - 체력 변화 델리게이트 바인딩
	OnHealthChanged.AddDynamic(this, &AAuraBossMonster::BeginBerserkMode);
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
	
	if (AuraAIController && AuraAIController->GetBlackboardComponent())
	{
		AuraAIController->GetBlackboardComponent()->SetValueAsBool("RoarEnd", false);
		AuraAIController->BehaviorTreeComponent->StopLogic(TEXT("Roar Started"));
	}
	
	// 플레이어 및 몬스터 무적
	if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
	{
		AuraGM->OnAllActorsInvincible.Broadcast(true);
	}
}

void AAuraBossMonster::OnRoarEnd(const FGameplayEventData* EventData)
{
	// 플레이어 컨트롤러에서 바인딩한 OnBossEventEnd 델리게이트 호출
	OnBossEventEnd.Broadcast(this);

	if (AuraAIController && AuraAIController->GetBlackboardComponent())
	{
		// 비헤이비어 트리 작동
		AuraAIController->RunBehaviorTree(BehaviorTree);
		AuraAIController->BehaviorTreeComponent->ResumeLogic(TEXT("Roar Ended"));
		AuraAIController->GetBlackboardComponent()->SetValueAsBool("RoarEnd", true);
	}
	
	// 플레이어 및 몬스터 무적 해제
	if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
	{
		AuraGM->OnAllActorsInvincible.Broadcast(false);
	}
}

void AAuraBossMonster::AddAbilityUpgradeOnBerserkMode()
{
	for (auto UpgradeClass : AbilityUpgradeClassToApplyBerserkMode)
	{
		if (UpgradeClass)
			AddAbilityUpgrade(UpgradeClass);
	}
}

void AAuraBossMonster::BeginBerserkMode(float NewHealth)
{
	float MaxHealth = UAuraAbilitySystemLibrary::GetAttributeValue(this, FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth);
	if (MaxHealth <= 0)
		return;

	// 체력이 일정 비율로 내려가면 광폭화 모드
	if (NewHealth / MaxHealth <= BeginBerserkRatio)
	{
		// 모든 소환수 제거
		RemoveAllMinions();
		
		// 블루프린트로 몽타주 실행, 머티리얼 변경
		OnBeginBerserkMontage.Broadcast();

		// 비헤이비어 트리 일시정지
		AuraAIController->BehaviorTreeComponent->StopLogic(TEXT("Roar Started"));
		
		// 블랙보드 플래그 변수 설정
		AuraAIController->GetBlackboardComponent()->SetValueAsBool("Berserk",true);

		// 한 번만 작동하도록 델리게이트 해제
		OnHealthChanged.RemoveDynamic(this, &AAuraBossMonster::BeginBerserkMode);

		// 광폭화 시 어빌리티 업그레이드 적용
		AddAbilityUpgradeOnBerserkMode();
	}
}
