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
#include "GameFramework/CharacterMovementComponent.h"


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
}

void AAuraEnemy::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 클라이언트는 복제로 제공받음
    if (!HasAuthority())
        return;

    AuraAIController = Cast<AAuraAIController>(NewController);

    // 블랙보드 초기화
    AuraAIController->GetBlackboardComponent()->InitializeBlackboard(*BehaviorTree->BlackboardAsset);
    
    // 비헤이비어 트리 작동
    AuraAIController->RunBehaviorTree(BehaviorTree);

    // 블랙보드 키 기본값 설정
    AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), false);

    // 근거리, 원거리 판정
    AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("RangedAttacker"), CharacterClass != ECharacterClass::Warrior);
}

void AAuraEnemy::HighlightActor()
{
    GetMesh()->SetRenderCustomDepth(true);
    GetMesh()->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
    Weapon->SetRenderCustomDepth(true);
    Weapon->SetCustomDepthStencilValue(CUSTOM_DEPTH_RED);
}

void AAuraEnemy::UnHighlightActor()
{
    GetMesh()->SetRenderCustomDepth(false);
    Weapon->SetRenderCustomDepth(false);
}

int32 AAuraEnemy::GetPlayerLevel()
{
    return Level;
}

void AAuraEnemy::Die()
{
    // 수명 설정
    SetLifeSpan(LifeSpan);

    if (AuraAIController)
    {
        // 블랙보드 키 기본값 설정
        AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("Dead"), true);
    }

    // 랙돌 효과와 무기 드랍
    Super::Die();
}

void AAuraEnemy::SetCombatTarget_Implementation(AActor* InCombatTarget)
{
    CombatTarget = InCombatTarget;
}

AActor* AAuraEnemy::GetCombatTarget_Implementation() const
{
    return CombatTarget;
}

void AAuraEnemy::BeginPlay()
{
    Super::BeginPlay();
    GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.f : BaseWalkSpeed;
    InitAbilityActorInfo();
    if (HasAuthority())
    {
        UAuraAbilitySystemLibrary::GiveStartupAbilities(this, AbilitySystemComponent, CharacterClass);
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

        OnHealthChanged.Broadcast(AuraAS->GetHealth());
        OnMaxHealthChanged.Broadcast(AuraAS->GetMaxHealth());
    }
}

void AAuraEnemy::HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
    // 태그가 있을 때만 작동
    bHitReacting = NewCount > 0;
    GetCharacterMovement()->MaxWalkSpeed = bHitReacting ? 0.f : BaseWalkSpeed;
    
    if (AuraAIController && AuraAIController->GetBlackboardComponent())
    {
        // 블랙보드 키 설정
        AuraAIController->GetBlackboardComponent()->SetValueAsBool(FName("HitReacting"), bHitReacting);
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
}

void AAuraEnemy::InitializeDefaultAttributes() const
{
    UAuraAbilitySystemLibrary::InitializeDefaultAttributes(this, CharacterClass, Level, AbilitySystemComponent);
}
