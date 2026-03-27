// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraBossMonster.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AI/AuraAIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/AuraCharacter.h"
#include "Components/SphereComponent.h"
#include "Game/AuraAudioSubsystem.h"
#include "Game/AuraGameModeBase.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AAuraBossMonster::AAuraBossMonster()
{
	bReplicates = true;
	
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetUsingAbsoluteRotation(true);
	SpringArm->SetRelativeRotation(FRotator(-30.f, -180.f, 0.f));
	SpringArm->TargetArmLength = 600.f;
	SpringArm->bDoCollisionTest = true;
	SpringArm->SetupAttachment(RootComponent);

	DeathCamera = CreateDefaultSubobject<UCameraComponent>("Camera");
	DeathCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	DeathCamera->bUsePawnControlRotation = false;
	DeathCamera->bAutoActivate = true;
	
	DetectSphere = CreateDefaultSubobject<USphereComponent>("DetectSphere");
	DetectSphere->SetSphereRadius(1200.f);
	DetectSphere->SetupAttachment(RootComponent);
}

void AAuraBossMonster::BossMontageBind()
{
	// 몽타주 태그와 콜백 바인딩
	auto* ASC = GetAbilitySystemComponent();
	if (ASC)
	{
		ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag("Event.Montage.Boss.RoarStart"))
		   .AddUObject(this, &AAuraBossMonster::OnRoarStart);
		
		ASC->GenericGameplayEventCallbacks.FindOrAdd(FGameplayTag::RequestGameplayTag("Event.Montage.Boss.RoarEnd"))
		   .AddUObject(this, &AAuraBossMonster::OnRoarEnd);
	}
}

void AAuraBossMonster::BeginPlay()
{
	Super::BeginPlay();
	
	if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
	{
		AuraGM->OnSetActorInvincible.Broadcast(this, true);
	}
	
	BossMontageBind();
}

void AAuraBossMonster::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
	
	// 보스 몬스터는 Event.Montage.Boss.RoarEnd 태그 이후에 AI가 작동
	if (!HasAuthority())
		return;

	// 광폭화 - 체력 변화 델리게이트 바인딩
	OnHealthChanged.AddDynamic(this, &AAuraBossMonster::BeginBerserkMode);
}

void AAuraBossMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AAuraBossMonster, bIsRoaring);
	DOREPLIFETIME(AAuraBossMonster, bBerserk);
}

void AAuraBossMonster::MultiCast_AttachDeathCam_Implementation()
{
	// 뗐다가 다시 붙임
	SpringArm->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	SpringArm->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("Pelvis"));
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bInheritYaw = false;
	
	SpringArm->SetRelativeRotation(FRotator(-45.f, 0.f, 0.f));
	SpringArm->TargetArmLength = 600.f;
	SpringArm->SocketOffset = FVector(0.f, 0.f, 100.f);
	SpringArm->bDoCollisionTest = false;
}

void AAuraBossMonster::Die(const FVector& DeathImpulse, AAuraCharacter* KilledBy)
{
	// 랙돌 효과와 무기 드랍
	Super::Die(DeathImpulse, KilledBy);
	
	// 카메라를 척추에 고정
	MultiCast_AttachDeathCam();
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
	OnBossEventStart.Broadcast(this);

	if (HasAuthority())
	{
		bIsRoaring = true;
        
		if (AuraAIController && AuraAIController->GetBlackboardComponent())
		{
			AuraAIController->GetBlackboardComponent()->SetValueAsBool("RoarEnd", false);
			AuraAIController->BehaviorTreeComponent->StopLogic(TEXT("Roar Started"));
		}
        
		if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
		{
			AuraGM->OnAllActorsInvincible.Broadcast(true);
		}
	}
		
	if (bBerserk)
		return;
	
	// 음악 재생
	if (auto* AudioSS = GetWorld()->GetSubsystem<UAuraAudioSubsystem>())
	{
		AudioSS->PlayMusicByTag_NoVariable(FGameplayTag::RequestGameplayTag("Sound.Background.Boss.Shaman.Phase1"));
	}
}

void AAuraBossMonster::OnRoarEnd(const FGameplayEventData* EventData)
{
	OnBossEventEnd.Broadcast(this);

	if (HasAuthority())
	{
		bIsRoaring = true;
		
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
		
		bBerserk = true;
		
		// 음악 재생
		if (auto* AudioSS = GetWorld()->GetSubsystem<UAuraAudioSubsystem>())
		{
			AudioSS->PlayMusicByTag_NoVariable(FGameplayTag::RequestGameplayTag("Sound.Background.Boss.Shaman.Phase2"));
		}
		
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

void AAuraBossMonster::OnRep_IsRoaring()
{
	if (bIsRoaring)
	{
		OnBossEventStart.Broadcast(this);
		
		// 광폭화 상태라면 실행 안함
		if (bBerserk)
			return;
		
		// 음악 재생
		if (auto* AudioSS = GetWorld()->GetSubsystem<UAuraAudioSubsystem>())
		{
			AudioSS->PlayMusicByTag_NoVariable(FGameplayTag::RequestGameplayTag("Sound.Background.Boss.Shaman.Phase1"));
		}
	}
	else
	{
		OnBossEventEnd.Broadcast(this);
	}
}

void AAuraBossMonster::OnRep_Berserk()
{
	// 블루프린트로 몽타주 실행, 머티리얼 변경
	OnBeginBerserkMontage.Broadcast();
	
	// 음악 재생
	if (auto* AudioSS = GetWorld()->GetSubsystem<UAuraAudioSubsystem>())
	{
		AudioSS->PlayMusicByTag_NoVariable(FGameplayTag::RequestGameplayTag("Sound.Background.Boss.Shaman.Phase2"));
	}
}
