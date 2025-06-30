// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerState.h"

#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"


AAuraPlayerState::AAuraPlayerState()
{
    // 서버 업데이트 빈도
    // GAS에 적용하기 위해 빈도를 더 빠르게 조정
    NetUpdateFrequency = 100.f;

    AbilitySystemComponent = CreateDefaultSubobject<UAuraAbilitySystemComponent>("AbilitySystemComponent");

    // 멀티에서의 복제
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>("AttributeSet");
}

void AAuraPlayerState::BeginPlay()
{
    Super::BeginPlay();
    
    // 초기화 완료
    OnPlayerStateInitialized.Broadcast(this);
    
    // if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent))
    // {
    //     AuraASC->AbilitiesInitializedDelegate.AddUObject(this, &AAuraPlayerState::HandleAbilitiesSet);
    // }
}

void AAuraPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAuraPlayerState, Level);
    DOREPLIFETIME(AAuraPlayerState, XP);
    DOREPLIFETIME(AAuraPlayerState, AttributePoints);
    DOREPLIFETIME(AAuraPlayerState, SpellPoints);
    DOREPLIFETIME(AAuraPlayerState, OwnedAbilityUpgradeTags);
}

void AAuraPlayerState::SetXP(int32 GainedXP)
{
    XP = GainedXP;
    OnXPChangedDelegate.Broadcast(XP);
}

void AAuraPlayerState::AddToXP(int32 GainedXP)
{
    XP += GainedXP;
    OnXPChangedDelegate.Broadcast(XP);
}

void AAuraPlayerState::SetLevel(int32 InLevel)
{
    Level = InLevel;
    OnLevelChangedDelegate.Broadcast(Level, false);
}

void AAuraPlayerState::AddToLevel(int32 InLevel)
{
    Level += InLevel;
    OnLevelChangedDelegate.Broadcast(Level, true);
}

void AAuraPlayerState::SetAttributePoints(int32 InAP)
{
    AttributePoints = InAP;
    OnAttributePointChangedDelegate.Broadcast(AttributePoints);
}

void AAuraPlayerState::AddToAttributePoints(int32 InAP)
{
    AttributePoints += InAP;
    OnAttributePointChangedDelegate.Broadcast(AttributePoints);
}

void AAuraPlayerState::SetSpellPoints(int32 InSP)
{
    SpellPoints = InSP;
    OnSpellPointChangedDelegate.Broadcast(SpellPoints);
}

void AAuraPlayerState::AddToSpellPoints(int32 InSP)
{
    SpellPoints += InSP;
    OnSpellPointChangedDelegate.Broadcast(SpellPoints);
}

void AAuraPlayerState::SetHealth(const float InHealth)
{
    if (UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AttributeSet))
    {
        AuraAS->SetHealth(InHealth);
    }
}

void AAuraPlayerState::SetMana(const float InMana)
{
    if (UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AttributeSet))
    {
        AuraAS->SetMana(InMana);
    }
}

void AAuraPlayerState::SetAbilityUpgradeTagContainer(const FGameplayTagContainer& InTagContainer)
{
    OwnedAbilityUpgradeTags = InTagContainer;
}

int32 AAuraPlayerState::GetUpgradeTagCount(FGameplayTag UpgradeTag)
{
    int TagCount = 0;
    
    if (!UpgradeTag.IsValid())
        return 0;

    for (auto& OwnedTag : OwnedAbilityUpgradeTags)
    {
        if (OwnedTag.MatchesTagExact(UpgradeTag))
            TagCount += 1;
    }
    
    return TagCount;
}

TArray<FGameplayTag> AAuraPlayerState::GetRandomActivatedAbilityTags_Three(const FGameplayTagContainer& ActivatedAbilityTags)
{
    TArray<FGameplayTag> ReturnTags;
    ReturnTags.Empty();
    
    if (ActivatedAbilityTags.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("No Activatable Abilities Exist"));
        return ReturnTags;
    }
    
    int32 ContainerNum = ActivatedAbilityTags.Num();

    int Int0 = UKismetMathLibrary::RandomIntegerInRange(0, ContainerNum-1);
    int Int1 = UKismetMathLibrary::RandomIntegerInRange(0, ContainerNum-1);
    int Int2 = UKismetMathLibrary::RandomIntegerInRange(0, ContainerNum-1);

    FGameplayTag AbilityTag0 = ActivatedAbilityTags.GetByIndex(Int0);
    FGameplayTag AbilityTag1 = ActivatedAbilityTags.GetByIndex(Int1);
    FGameplayTag AbilityTag2 = ActivatedAbilityTags.GetByIndex(Int2);
    
    while ( !AbilityTag0.IsValid() || !AbilityTag1.IsValid() || !AbilityTag2.IsValid() )
    {
        Int0 = UKismetMathLibrary::RandomIntegerInRange(0, ContainerNum-1);
        Int1 = UKismetMathLibrary::RandomIntegerInRange(0, ContainerNum-1);
        Int2 = UKismetMathLibrary::RandomIntegerInRange(0, ContainerNum-1);
        
        AbilityTag0 = ActivatedAbilityTags.GetByIndex(Int0);
        AbilityTag1 = ActivatedAbilityTags.GetByIndex(Int1);
        AbilityTag2 = ActivatedAbilityTags.GetByIndex(Int2);
    }
    
    ReturnTags.Add(AbilityTag0);
    ReturnTags.Add(AbilityTag1);
    ReturnTags.Add(AbilityTag2);

    return ReturnTags;
}

TArray<FGameplayTag> AAuraPlayerState::GetRandomUpgradeTagsForActivatedAbility_Three()
{
    const TArray<FGameplayTag> ActivatedAbilityTags = GetAllActiveAbilityTags();
    
    TArray<FGameplayTag> RandomUpgradeTags;
    RandomUpgradeTags.Empty();
    
    // 주어진 어빌리티 태그에 대해 랜덤한 업그레이드 태그 뽑기
    if (UAbilityUpgradeInfo* Info = UAuraAbilitySystemLibrary::GetAbilityUpgradeInfo(this))
    {
        int ActivatedAbilityTagsNum = ActivatedAbilityTags.Num();
        int RandValue0 = UKismetMathLibrary::RandomIntegerInRange(0, ActivatedAbilityTagsNum-1);
        int RandValue1 = UKismetMathLibrary::RandomIntegerInRange(0, ActivatedAbilityTagsNum-1);
        int RandValue2 = UKismetMathLibrary::RandomIntegerInRange(0, ActivatedAbilityTagsNum-1);

        // 랜덤으로 뽑은 어빌리티 태그는 중복 가능 -> 파이어볼트 업그레이드 2개 노출 가능
        const auto Tag0 = ActivatedAbilityTags[RandValue0];
        const auto Tag1 = ActivatedAbilityTags[RandValue1];
        const auto Tag2 = ActivatedAbilityTags[RandValue2];
        
        FGameplayTag UpgradeTag0;
        FGameplayTag UpgradeTag1;
        FGameplayTag UpgradeTag2;

        // 루프 함수 제한
        int CurrentAttempt = 0;
        
        // 동일한 태그가 없을 때까지 뽑기
        while (UpgradeTag0 == UpgradeTag1 || UpgradeTag1 == UpgradeTag2 || UpgradeTag0 == UpgradeTag2)
        {
            UpgradeTag0 = Info->GetRandomUpgradeTagForAbility(Tag0);
            UpgradeTag1 = Info->GetRandomUpgradeTagForAbility(Tag1);
            UpgradeTag2 = Info->GetRandomUpgradeTagForAbility(Tag2);

            CurrentAttempt++;

            if (CurrentAttempt > 10)
                break;
        }
        RandomUpgradeTags.Add(UpgradeTag0);
        RandomUpgradeTags.Add(UpgradeTag1);
        RandomUpgradeTags.Add(UpgradeTag2);
        
        // AuraGameModeBase에서 바인딩
        OnRandomUpgradeTagsGeneratedDelegate.Broadcast(this, RandomUpgradeTags);
    }
    return RandomUpgradeTags;
}

void AAuraPlayerState::HandleAbilitiesSet()
{
    // 어빌리티 부여 종료 후 호출되는 콜백 함수
    
}

TArray<FGameplayTag> AAuraPlayerState::GetAllActiveAbilityTags() const
{
    TArray<FGameplayTag> AllActiveTags;

    auto AuraTags = FAuraGameplayTags::Get();
    
    // 활성화된 어빌리티
    TArray<FGameplayAbilitySpec> ActiveSpecs = AbilitySystemComponent->GetActivatableAbilities();

    for (FGameplayAbilitySpec& Spec : ActiveSpecs)
    {
        FGameplayTagContainer AbilityTagContainer = Spec.Ability->AbilityTags;
        if (AbilityTagContainer.HasTag(AuraTags.Abilities_None))
            continue;

        if (AbilityTagContainer.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities.Fire")))
            || AbilityTagContainer.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities.Arcane")))
            || AbilityTagContainer.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities.Lightning")))
            )
        {
            for (auto Tag : AbilityTagContainer)
            {
                if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities"))))
                {
                    // 어빌리티 태그라면 추가
                    AllActiveTags.Add(Tag);
                }
            }
        }
    }
    
    return AllActiveTags;
}

void AAuraPlayerState::Server_AddAbilityUpgradeTag_Implementation(FGameplayTag UpgradeTag)
{
    OwnedAbilityUpgradeTags.AddTag(UpgradeTag);
}

void AAuraPlayerState::Server_RemoveAbilityUpgradeTag_Implementation(FGameplayTag UpgradeTag)
{
    OwnedAbilityUpgradeTags.RemoveTag(UpgradeTag);
}

void AAuraPlayerState::OnRep_AbilityUpgradeTags()
{
    OnAbilityUpgradeTagsChangedDelegate.Broadcast();
}

void AAuraPlayerState::OnRep_Level(int32 OldLevel)
{
    // 블루프린트로 전달
    OnLevelChangedDelegate.Broadcast(Level, true);
}

void AAuraPlayerState::OnRep_XP(int32 OldXP)
{
    // 블루프린트로 전달
    OnXPChangedDelegate.Broadcast(XP);
}

void AAuraPlayerState::OnRep_AttributePoint(int32 OldAttributePoint)
{
    OnAttributePointChangedDelegate.Broadcast(AttributePoints);
}

void AAuraPlayerState::OnRep_SpellPoint(int32 OldSpellPoint)
{
    OnSpellPointChangedDelegate.Broadcast(SpellPoints);
}
