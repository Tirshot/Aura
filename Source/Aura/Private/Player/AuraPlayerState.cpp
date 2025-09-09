// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerState.h"

#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Net/UnrealNetwork.h"
#include "UI/HUD/AuraHUD.h"


AAuraPlayerState::AAuraPlayerState()
{
    // 서버 업데이트 빈도
    // GAS에 적용하기 위해 빈도를 더 빠르게 조정
    SetNetUpdateFrequency(100.f);

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
    DOREPLIFETIME(AAuraPlayerState, ReplicatedCardInfo);
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

void AAuraPlayerState::AddToLevelOne()
{
    Level += 1;
    OnLevelChangedDelegate.Broadcast(Level, true);
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

void AAuraPlayerState::SetUpgradeCardInfo(const TArray<FAuraAbilityUpgradeInfo>& NewCard)
{
    ReplicatedCardInfo = NewCard;
    OnUpgradeCardsInitializedDelegate.Broadcast(ReplicatedCardInfo);
}

void AAuraPlayerState::SetAbilityUpgradeTagContainer(const TMap<FGameplayTag, int32>& InTagContainer)
{
    OwnedAbilityUpgradeTags = InTagContainer;
}

void AAuraPlayerState::AddUpgradeTag(const FGameplayTag& Tag)
{
    // 태그 추가
    int32& CountRef = OwnedAbilityUpgradeTags.FindOrAdd(Tag);

    // 값 1 증가
    CountRef++;
}

void AAuraPlayerState::RemoveUpgradeTag(const FGameplayTag& Tag)
{
    // 태그 제거
    int32* Count = OwnedAbilityUpgradeTags.Find(Tag);

    if (Count != nullptr)
    {
        // 중첩 갯수 감소
        *Count -= 1;

        // 중첩 갯수가 0이 되면 제거
        if (*Count < 1)
        {
            OwnedAbilityUpgradeTags.Remove(Tag);
            OnAbilityUpgradeTagsChangedDelegate.Broadcast(Tag, 0);
            return;
        }
    }
}

int32 AAuraPlayerState::GetUpgradeTagCount(FGameplayTag UpgradeTag)
{
    int TagCount = 0;
    
    if (!UpgradeTag.IsValid())
        return 0;

    TagCount = *OwnedAbilityUpgradeTags.Find(UpgradeTag);
    
    return TagCount;
}

bool AAuraPlayerState::HasUpgradeTag(FGameplayTag UpgradeTag)
{
    return OwnedAbilityUpgradeTags.Find(UpgradeTag) ? true : false;
}

void AAuraPlayerState::HandleAbilitiesSet()
{
    // 어빌리티 부여 종료 후 호출되는 콜백 함수
    
}

TArray<FGameplayTag> AAuraPlayerState::GetAllAbilityTags()
{
    return FAuraGameplayTags::Get().GameplayAbilitiesTags;
}

TArray<FGameplayTag> AAuraPlayerState::GetAllActiveAbilityTags() const
{
    TArray<FGameplayTag> AllActiveTags;

    auto& AuraTags = FAuraGameplayTags::Get();
    
    // 활성화된 어빌리티
    TArray<FGameplayAbilitySpec> ActiveSpecs = AbilitySystemComponent->GetActivatableAbilities();

    for (FGameplayAbilitySpec& Spec : ActiveSpecs)
    {
        FGameplayTagContainer AbilityTagContainer = Spec.Ability->GetAssetTags();
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

TArray<FGameplayTag> AAuraPlayerState::GetAllInActiveAbilityTags() const
{
    TArray<FGameplayTag> AllTags = FAuraGameplayTags::Get().GameplayAbilitiesTags;
    TArray<FGameplayTag> AllActiveTags;

    auto& AuraTags = FAuraGameplayTags::Get();
    
    // 활성화된 어빌리티
    TArray<FGameplayAbilitySpec> ActiveSpecs = AbilitySystemComponent->GetActivatableAbilities();

    for (FGameplayAbilitySpec& Spec : ActiveSpecs)
    {
        FGameplayTagContainer AbilityTagContainer = Spec.Ability->GetAssetTags();
        if (AbilityTagContainer.HasTag(AuraTags.Abilities_None))
            continue;

        for (auto Tag : AbilityTagContainer)
        {
            if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities"))))
            {
                // 어빌리티 태그라면 추가
                AllActiveTags.Add(Tag);
            }
        }
    }

    // 모든 어빌리티 태그에서 활성화된 태그를 제거
    for (auto ActiveTag : AllActiveTags)
    {
        AllTags.Remove(ActiveTag);
    }

    return AllTags;
}

void AAuraPlayerState::GetRandomAttributeUpgrade()
{
    // 랜덤 속성 뽑기
    TArray<FGameplayTag> AttributeTags = FAuraGameplayTags::Get().AttributesTags;
    int32 RandAttributeNum = FMath::RandRange(0, AttributeTags.Num() - 1);
    
    // 랜덤 수치 뽑기
    
}

void AAuraPlayerState::Server_AddAbilityUpgradeTag_Implementation(FGameplayTag UpgradeTag)
{
    // 비 보유 중인 어빌리티라면 새로 습득
    // 보유 중인 어빌리티라면 레벨 상승
    // 어빌리티 군 업그레이드는 Abilties.Fire / Abilities.Lightning / Abilities.Arcane
    if (auto* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        AuraASC->AddCharacterAbility(UpgradeTag);
    }

    // 만약 어빌리티 획득 업그레이드(또는 레벨업 업그레이드)라면 업그레이드 태그로 저장하지 않음
    if (UpgradeTag.RequestDirectParent().MatchesTag(FGameplayTag::RequestGameplayTag("Abilities")))
        return;
    
    // 보유중인 어빌리티 업그레이드 배열에 추가
    AddUpgradeTag(UpgradeTag);
}

void AAuraPlayerState::Server_RemoveAbilityUpgradeTag_Implementation(FGameplayTag UpgradeTag)
{
    // if (auto* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
    // {
    //     AuraASC->RemoveCharacterAbility(UpgradeTag);
    // }
    
    // 보유중인 어빌리티 업그레이드 배열에서 제거
    RemoveUpgradeTag(UpgradeTag);
}

// void AAuraPlayerState::OnRep_AbilityUpgradeTags()
// {
//     // 업그레이드 목록 UI로 전송
//     OnAbilityUpgradeTagsChangedDelegate.Broadcast(OwnedAbilityUpgradeTags);
// }

void AAuraPlayerState::OnRep_UpgradeCardInfo()
{
    OnUpgradeCardsInitializedDelegate.Broadcast(ReplicatedCardInfo);
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
    // 사용 가능한 포인트 있음 알리기
    if (OldAttributePoint > 0)
    {
        UAuraAbilitySystemLibrary::ApplyMessageTagEffectToSelf(
            FGameplayTag::RequestGameplayTag("Message.LevelUp"),
            AbilitySystemComponent->GetAvatarActor());
    }
    else
    {
        UAuraAbilitySystemLibrary::RemoveMessageTagEffectToSelf(
            AbilitySystemComponent,
            FGameplayTag::RequestGameplayTag("Message.LevelUp"));
    }
    
    //
    OnAttributePointChangedDelegate.Broadcast(AttributePoints);
}

void AAuraPlayerState::OnRep_SpellPoint(int32 OldSpellPoint)
{
    // 사용 가능한 포인트 있음 알리기
    if (OldSpellPoint > 0)
    {
        UAuraAbilitySystemLibrary::ApplyMessageTagEffectToSelf(
            FGameplayTag::RequestGameplayTag("Message.LevelUp"),
            AbilitySystemComponent->GetAvatarActor());
    }
    else
    {
        UAuraAbilitySystemLibrary::RemoveMessageTagEffectToSelf(
            AbilitySystemComponent,
            FGameplayTag::RequestGameplayTag("Message.LevelUp"));
    }

    //
    OnSpellPointChangedDelegate.Broadcast(SpellPoints);
}
