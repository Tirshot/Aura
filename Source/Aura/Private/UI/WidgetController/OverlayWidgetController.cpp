// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "Player/AuraPlayerState.h"
#include "AuraGameplayTags.h"

void UOverlayWidgetController::BroadcastInitialValues()
{
    OnHealthChanged.Broadcast(GetAuraAS()->GetHealth());
    OnMaxHealthChanged.Broadcast(GetAuraAS()->GetMaxHealth());

    OnManaChanged.Broadcast(GetAuraAS()->GetMana());
    OnMaxManaChanged.Broadcast(GetAuraAS()->GetMaxMana());
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{
    GetAuraPS()->OnXPChangedDelegate.AddUObject(this, &UOverlayWidgetController::OnXPChanged);
    GetAuraPS()->OnLevelChangedDelegate.AddLambda([this](int32 NewLevel)
        {
            OnPlayerLevelChangedDelegate.Broadcast(NewLevel);
        });

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        GetAuraAS()->GetHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnHealthChanged.Broadcast(Data.NewValue);
            }
        );

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        GetAuraAS()->GetMaxHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxHealthChanged.Broadcast(Data.NewValue);
            }
        );

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        GetAuraAS()->GetManaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnManaChanged.Broadcast(Data.NewValue);
            }
        );

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        GetAuraAS()->GetMaxManaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxManaChanged.Broadcast(Data.NewValue);
            }
        );

    if (GetAuraASC())
    {
        GetAuraASC()->AbilityEquipped.AddUObject(this, &UOverlayWidgetController::OnAbilityEquipped);
        if (GetAuraASC()->bStartupAbilitiesGiven)
        {
            // 콜백 함수 바인드 필요 없이 바로 호출
            BroadcastAbilityInfo();
        }
        else
        {
            // 어빌리티 부여 이전이면 델리게이트에 함수 바인딩
            GetAuraASC()->AbilitiesGivenDelegate.AddUObject(this, &UOverlayWidgetController::BroadcastAbilityInfo);
        }

        // 플로팅 메세지
        GetAuraASC()->EffectAssetTags.AddLambda(
            [this](const FGameplayTagContainer& AssetTags)
            {   /* 멤버 변수에 접근하려면 캡쳐 */
                for (const FGameplayTag& Tag : AssetTags)
                {
                    // Tag = Massage.HealthPotion
                    // "Message.HealthPotion".MatchesTag("Message")는 True
                    // "Message".MatchesTag("Message.HealthPotion")는 False
                    // 매개변수로 전달된 태그가 전부 같아야 함
                    FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));
                    if (Tag.MatchesTag(MessageTag))
                    {
                        const FUIWidgetRow* Row = GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag);
                        MessageWidgetRowDelegate.Broadcast(*Row);
                    }
                }
            }
        );
    }
}

void UOverlayWidgetController::OnXPChanged(int32 NewXP)
{
    // 레벨업 정보 가져옴
    const ULevelUpInfo* LevelUpInfo = GetAuraPS()->LevelUpInfo;

    checkf(LevelUpInfo, TEXT("Can't Found LevelUpInfo. Fill out AuraPlayerState Blueprint"));

    // 경험치량 증가로 인해 달성한 레벨
    const int32 Level = LevelUpInfo->FindLevelForXP(NewXP);
    const int32 MaxLevel = LevelUpInfo->LevelUpInformation.Num();

    // 증가량
    if (Level <= MaxLevel && Level > 0)
    {
        const int32 LevelUpRequirement = LevelUpInfo->LevelUpInformation[Level].LevelUpRequirement;
        const int32 PrevLevelUpRequirement = LevelUpInfo->LevelUpInformation[Level - 1].LevelUpRequirement;

        // 현재 레벨의 끝 값 - 현재 레벨의 시작 값(==이전 레벨의 끝 값)
        const int32 Delta = LevelUpRequirement - PrevLevelUpRequirement;

        // 지금 레벨 안에서 경험치 진행값
        const int32 XPForThisLevel = NewXP - PrevLevelUpRequirement;

        const float XPBarPercent = static_cast<float>(XPForThisLevel) / Delta;

        OnXPPercentChanged.Broadcast(XPBarPercent);
    }
}

void UOverlayWidgetController::OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PrevSlot)
{
    const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

    FAuraAbilityInfo LastSlotInfo;
    LastSlotInfo.StatusTag = GameplayTags.Abilities_Status_Unlocked;
    LastSlotInfo.InputTag = PrevSlot;
    LastSlotInfo.AbilityTag = GameplayTags.Abilities_None;

    // 변경할 슬롯에 이미 어빌리티가 있다면 빈 어빌리티 정보를 보냄
    AbilityInfoDelegate.Broadcast(LastSlotInfo);

    // 변경할 슬롯에 선택한 어빌리티의 정보를 채움
    FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
    Info.StatusTag = Status;
    Info.InputTag = Slot;
    Info.AbilityTag = AbilityTag;
    AbilityInfoDelegate.Broadcast(Info);
}
