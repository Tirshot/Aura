// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerState.h"

#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Net/UnrealNetwork.h"


void FOwnedAbilityUpgrade::PostReplicatedAdd(const struct FOwnedAbilityUpgradeList& InArraySerializer)
{
    
}

void FOwnedAbilityUpgrade::PostReplicatedChange(const struct FOwnedAbilityUpgradeList& InArraySerializer)
{
    
}

void FOwnedAbilityUpgrade::PreReplicatedRemove(const struct FOwnedAbilityUpgradeList& InArraySerializer)
{
    
}

bool FOwnedAbilityUpgradeList::FindOwnedAbilityUpgrade(const FGameplayTag& InTag)
{
    for (auto& OwnedAbilityUpgrade : OwnedAbilityUpgrades)
    {
        if (OwnedAbilityUpgrade.UpgradeTag == InTag)
        {
            return true;
        }
    }
    return false;
}

FOwnedAbilityUpgrade* FOwnedAbilityUpgradeList::GetOwnedAbilityUpgrade(const FGameplayTag& InTag)
{
    for (auto& OwnedAbilityUpgrade : OwnedAbilityUpgrades)
    {
        if (OwnedAbilityUpgrade.UpgradeTag == InTag)
        {
            return &OwnedAbilityUpgrade;
        }
    }
    return nullptr;
}

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
}

void AAuraPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAuraPlayerState, Level);
    DOREPLIFETIME(AAuraPlayerState, XP);
    DOREPLIFETIME(AAuraPlayerState, AttributePoints);
    DOREPLIFETIME(AAuraPlayerState, SpellPoints);
    DOREPLIFETIME(AAuraPlayerState, OwnedAbilityUpgradeList);
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
    AddToLevel(1);
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

void AAuraPlayerState::SetAbilityUpgradeTagContainer(const FOwnedAbilityUpgradeList& InOwnedAbilityUpgradeList)
{
    OwnedAbilityUpgradeList = InOwnedAbilityUpgradeList;
    OwnedAbilityUpgradeList.MarkArrayDirty();
}

void AAuraPlayerState::ResetAttributesToBaseValue()
{
    for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
    {
        if (FGameplayAttribute::IsGameplayAttributeDataProperty(*It))
        {
            FGameplayAttributeData* AttributeData = It->ContainerPtrToValuePtr<FGameplayAttributeData>(this);
            AttributeData->SetBaseValue(AttributeData->GetCurrentValue());
            AttributeData->SetCurrentValue(AttributeData->GetCurrentValue());
        }
    }
}

void AAuraPlayerState::Server_SyncPlayerStatFromClient_Implementation(int32 InLevel, int32 InXP,
                                                                      int32 InAttributePoints, int32 InSpellPoints)
{
    SetLevel(InLevel);
    SetXP(InXP);
    SetAttributePoints(InAttributePoints);
    SetSpellPoints(InSpellPoints);
}

void AAuraPlayerState::Server_SyncPlayerUpgradeListFromClient_Implementation(
    const FOwnedAbilityUpgradeList& UpgradeList)
{
    SetAbilityUpgradeTagContainer(UpgradeList);
}

void AAuraPlayerState::AddUpgradeTag(const FGameplayTag& Tag)
{
    int32 UpgradeStack = 0;
    
    if (FOwnedAbilityUpgrade* OwnedAbilityUpgrade = OwnedAbilityUpgradeList.GetOwnedAbilityUpgrade(Tag))
    {
        // 이미 존재하는 업그레이드
        OwnedAbilityUpgrade->UpgradeStack++;
        UpgradeStack = OwnedAbilityUpgrade->UpgradeStack;
        OnAbilityUpgradeTagsChangedDelegate.Broadcast(Tag, UpgradeStack);
        OwnedAbilityUpgradeList.MarkArrayDirty();
    }
    else
    {
        // 존재하지 않으면 추가
        OwnedAbilityUpgradeList.OwnedAbilityUpgrades.Add(Tag);
        OnAbilityUpgradeTagsChangedDelegate.Broadcast(Tag, 1);
        OwnedAbilityUpgradeList.MarkArrayDirty();
    }
}

void AAuraPlayerState::RemoveUpgradeTag(const FGameplayTag& Tag)
{
    // 태그 제거

    if (FOwnedAbilityUpgrade* OwnedAbilityUpgrade = OwnedAbilityUpgradeList.GetOwnedAbilityUpgrade(Tag))
    {
        int32 UpgradeStack = 0;
        
        // 이미 존재하는 업그레이드
        OwnedAbilityUpgrade->UpgradeStack--;
        UpgradeStack = FMath::Max(0, OwnedAbilityUpgrade->UpgradeStack);
    
        // 업그레이드 스택이 0보다 작으면 업그레이드 제거
        if (UpgradeStack <= 0)
        {
            OwnedAbilityUpgradeList.OwnedAbilityUpgrades.RemoveAll([&Tag](const FOwnedAbilityUpgrade& Upgrade)
            {
                return Upgrade.UpgradeTag.MatchesTagExact(Tag);
            });
        }
        OwnedAbilityUpgradeList.MarkArrayDirty();
        OnAbilityUpgradeTagsChangedDelegate.Broadcast(Tag, UpgradeStack);
    }
}

int32 AAuraPlayerState::GetUpgradeTagCount(FGameplayTag UpgradeTag)
{
    if (!UpgradeTag.IsValid())
        return 0;

    if (auto* Upgrade = OwnedAbilityUpgradeList.GetOwnedAbilityUpgrade(UpgradeTag))
    {
        return Upgrade->UpgradeStack;
    }

    return 0;
}

bool AAuraPlayerState::HasUpgradeTag(FGameplayTag UpgradeTag)
{
    return OwnedAbilityUpgradeList.FindOwnedAbilityUpgrade(UpgradeTag);
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

        if (Spec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Eligible)
            || Spec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Locked))
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
                    if (UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this))
                    {
                        FAuraAbilityInfo* AuraAbilityInfo = AbilityInfo->FindAbilityInfoForTag(Tag);
                        
                        if (AuraAbilityInfo->StatusTag.MatchesTag(AuraTags.Abilities_Status_Eligible)
                           || AuraAbilityInfo->StatusTag.MatchesTag(AuraTags.Abilities_Status_Equipped))
                        {
                            // 어빌리티 태그라면 추가
                            AllActiveTags.Add(Tag);
                        }
                    }
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

void AAuraPlayerState::Server_AddAbilityUpgradeTag_Implementation(FGameplayTag UpgradeTag)
{
    // 비 보유 중인 어빌리티라면 새로 습득, 보유 중인 어빌리티라면 레벨 상승
    
    // 어빌리티 군 업그레이드는 Abilties.Fire / Abilities.Lightning / Abilities.Arcane
    // 만약 어빌리티 획득 업그레이드(또는 레벨업 업그레이드)라면 업그레이드 태그로 저장하지 않음
    if (UpgradeTag.RequestDirectParent().MatchesTag(FGameplayTag::RequestGameplayTag("Abilities")))
    {
        if (auto* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
        {
            AuraASC->AddCharacterAbilityByTag(UpgradeTag);
            AuraASC->AbilityStatusChanged.Broadcast(UpgradeTag, FAuraGameplayTags::Get().Abilities_Status_Equipped, 0);
        }
    }
    
    // 보유중인 어빌리티 업그레이드 배열에 추가
    AddUpgradeTag(UpgradeTag);
}

void AAuraPlayerState::Server_RemoveAbilityUpgradeTag_Implementation(FGameplayTag UpgradeTag)
{
    // 어빌리티 획득 업그레이드(또는 레벨업 업그레이드) 제거
    if (UpgradeTag.RequestDirectParent().MatchesTag(FGameplayTag::RequestGameplayTag("Abilities")))
    {
        if (auto* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
        {
            AuraASC->RemoveCharacterAbilityByTag(UpgradeTag, 1);
        }
    }
    
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
