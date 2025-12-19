// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraEnemy.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/Widget/AuraUserWidget.h"
#include "AuraGameplayTags.h"
#include "Aura/Aura.h"
#include "AI/AuraAIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Character/AuraBossMonster.h"
#include "Game/AuraGameModeBase.h"


AAuraEnemy::AAuraEnemy()
{
    GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    AbilitySystemComponent = CreateDefaultSubobject<UAuraAbilitySystemComponent>("AbilitySystemComponent");

    // 멀티에서의 복제
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bUseControllerDesiredRotation = true;

    AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>("AttributeSet");

    HealthBar = CreateDefaultSubobject<UWidgetComponent>("HealthBar");
    HealthBar->SetupAttachment(GetRootComponent());
    
    GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
    GetMesh()->MarkRenderStateDirty();
    Weapon->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
    Weapon->MarkRenderStateDirty();
}

void AAuraEnemy::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 클라이언트는 복제로 제공받음
    if (!HasAuthority())
        return;

    if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        AuraGameMode->AddMonsterToArray(this);
    }

    AuraAIController = Cast<AAuraAIController>(NewController);

    // 블랙보드 초기화
    AuraAIController->GetBlackboardComponent()->InitializeBlackboard(*BehaviorTree->BlackboardAsset);

    if (AAuraBossMonster* BossCharacter = Cast<AAuraBossMonster>(this))
    {
        // 보스 몬스터는 몽타주 이벤트 이후에 비헤이비어 트리 활성화
    }
    else
    {
        // 비헤이비어 트리 작동
        AuraAIController->RunBehaviorTree(BehaviorTree);
    }

    // 블랙보드 키 기본값 설정
    AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), false);

    // 근거리, 원거리 판정
    AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("RangedAttacker"), CharacterClass != ECharacterClass::Warrior);
}

void AAuraEnemy::HighlightActor_Implementation()
{
    GetMesh()->SetRenderCustomDepth(true);
    Weapon->SetRenderCustomDepth(true);
}

void AAuraEnemy::UnHighlightActor_Implementation()
{
    GetMesh()->SetRenderCustomDepth(false);
    Weapon->SetRenderCustomDepth(false);
}

void AAuraEnemy::SetMoveToLocation_Implementation(FVector& OutDestination)
{
    // 절대 건드리지 마시오
}

int32 AAuraEnemy::GetCharacterLevel_Implementation()
{
    return Level;
}

void AAuraEnemy::Die(const FVector& DeathImpulse, AAuraCharacter* KilledBy)
{
    // 랙돌 효과와 무기 드랍
    Super::Die(DeathImpulse, KilledBy);

    HealthBar->DestroyComponent();
    
    if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        AuraGameMode->RemoveMonsterFromArray(this);
    }

    // 수명 설정
    SetLifeSpan(LifeSpan);

    if (AuraAIController)
    {
        // 블랙보드 키 기본값 설정
        AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
    }
    
    // 포션, 회복 수정 아이템 드랍
    SpawnLoot();
    
    // 아이템 드랍
    if (HasAuthority())
    {
        if (AAuraGameModeBase* GM = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
        {
            GM->DropItemOnMonsterDied(this, KilledBy);
        }
    }
}

void AAuraEnemy::SetCombatTarget_Implementation(AActor* InCombatTarget)
{
    CombatTarget = InCombatTarget;
}

AActor* AAuraEnemy::GetCombatTarget_Implementation() const
{
    return CombatTarget;
}

void AAuraEnemy::SetIsBeingShocked_Implementation(bool bInShock)
{
    // 보스는 쇼크로 인한 경직 무시
    if (ActorHasTag("Boss"))
        return;
    
    bIsBeingShock = bInShock;

    BeingShockedTagChanged();
}

bool AAuraEnemy::IsXPOverridden_Implementation() const
{
    return bIsXPOverride;
}

float AAuraEnemy::GetXPOverriddenValue_Implementation() const
{
    return XPOverrideValue;
}

void AAuraEnemy::BeginPlay()
{
    Super::BeginPlay();

    InitAbilityActorInfo();
    if (HasAuthority())
    {
        // 초기 어빌리티 부여
        UAuraAbilitySystemLibrary::GiveStartupAbilities(this, AbilitySystemComponent, CharacterClass);
        UAuraAbilitySystemLibrary::GiveStartupPassiveAbilities(this, AbilitySystemComponent, CharacterClass);

        // 초기 어빌리티 업그레이드 부여
        for (auto UpgradeClass : AbilityUpgradeClassToApply)
        {
            if (UpgradeClass)
            {
                // 게임플레이 이펙트 스펙 생성 후 적용
                AddAbilityUpgrade(UpgradeClass);
            }
        }
    }
    
    if (UAuraUserWidget* AuraUserWidget = Cast<UAuraUserWidget>(HealthBar->GetUserWidgetObject()))
    {
        AuraUserWidget->SetWidgetController(this);
    }

    if (const UAuraAttributeSet* AuraAS = CastChecked<UAuraAttributeSet>(AttributeSet))
    {
        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAS->GetHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data) 
            {
                OnHealthChanged.Broadcast(Data.NewValue);
            }
        );

        AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AuraAS->GetMaxHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxHealthChanged.Broadcast(Data.NewValue);
            }
        );
        // 델리게이트 - 태그, 태그 갯수
        AbilitySystemComponent->RegisterGameplayTagEvent(FAuraGameplayTags::Get().Effects_HitReact, EGameplayTagEventType::NewOrRemoved).AddUObject(
            this,
            &AAuraEnemy::HitReactTagChanged
        );

        // 이동 속도는 어트리뷰트를 사용함
        // 이동 속도 설정
        GetCharacterMovement()->MaxWalkSpeed = AuraAS->GetMovementSpeed();

        OnHealthChanged.Broadcast(AuraAS->GetHealth());
        OnMaxHealthChanged.Broadcast(AuraAS->GetMaxHealth());
    }
}

void AAuraEnemy::HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
    if (const UAuraAttributeSet* AuraAS = CastChecked<UAuraAttributeSet>(AttributeSet))
    {
        // 태그가 있을 때만 작동
        bHitReacting = NewCount > 0;
        GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.f : AuraAS->GetMovementSpeed();
    
        if (AuraAIController && AuraAIController->GetBlackboardComponent())
        {
            // 블랙보드 키 설정
            AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), bHitReacting);
        }
    }
}

void AAuraEnemy::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
    Super::StunTagChanged(CallbackTag, NewCount);

    if (AuraAIController && AuraAIController->GetBlackboardComponent())
    {
        // 블랙보드 키 설정
        AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("Stunned"), bIsStunned);
    }
}

void AAuraEnemy::BeingShockedTagChanged()
{
    Super::BeingShockedTagChanged();

    if (AuraAIController && AuraAIController->GetBlackboardComponent())
    {
        // 블랙보드 키 설정
        AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("BeingShocked"), bIsBeingShock);
    }
}

void AAuraEnemy::InitAbilityActorInfo()
{
    AbilitySystemComponent->InitAbilityActorInfo(this, this);
    Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();

    if (HasAuthority())
    {
        InitializeDefaultAttributes();
    }
    // ASC가 초기화 되었음을 알리는 델리게이트 호출
    OnASCRegistered.Broadcast(AbilitySystemComponent);

    // 스턴 태그 대기
    AbilitySystemComponent->RegisterGameplayTagEvent(FAuraGameplayTags::Get().Debuff_Stun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AAuraEnemy::StunTagChanged);
}

void AAuraEnemy::InitializeDefaultAttributes() const
{
    UAuraAbilitySystemLibrary::InitializeDefaultAttributes(this, CharacterClass, Level, AbilitySystemComponent);
}

void AAuraEnemy::SpawnDropItem()
{
    // 아이템 그룹 별 - 소모품, 장비, 기타 등 등장 확률
    
    // 캐릭터 클래스에 맞는 아이템 - 그룹 내 아이템 별 등장 확률
    
}

void AAuraEnemy::AddAbilityUpgrade(TSubclassOf<UGameplayEffect> AbilityUpgradeClass)
{
    if (AbilityUpgradeClass)
    {
        // 게임플레이 이펙트 스펙 생성 후 적용
        FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
        FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(AbilityUpgradeClass, 1.0f, ContextHandle);

        AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    }
}

void AAuraEnemy::RemoveAbilityUpgrade(TSubclassOf<UGameplayEffect> AbilityUpgradeClass)
{
    if (AbilityUpgradeClass)
    {
        // 게임플레이 이펙트를 ASC에서 제거
        AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(AbilityUpgradeClass, nullptr);
    }
}
