// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "Player/AuraPlayerState.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

void UOverlayWidgetController::BroadcastInitialValues()
{
    OnHealthChanged.Broadcast(GetAuraAS()->GetHealth());
    OnMaxHealthChanged.Broadcast(GetAuraAS()->GetMaxHealth());

    OnManaChanged.Broadcast(GetAuraAS()->GetMana());
    OnMaxManaChanged.Broadcast(GetAuraAS()->GetMaxMana());
    
    int32 Level = GetAuraPS()->GetCharacterLevel();
    OnPlayerLevelChangedDelegate.Broadcast(Level, false);
    GetAuraPS()->OnXPChangedDelegate.Broadcast(GetAuraPS()->GetXP());
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{
    GetAuraPS()->OnXPChangedDelegate.AddDynamic(this, &UOverlayWidgetController::OnXPChanged);
    GetAuraPS()->OnLevelChangedDelegate.AddLambda([this](int32 NewLevel, bool bLevelUp)
        {
            OnPlayerLevelChangedDelegate.Broadcast(NewLevel, bLevelUp);
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

    if (auto* AuraASC = GetAuraASC())
    {
        AuraASC->AbilityEquipped.AddDynamic(this, &UOverlayWidgetController::OnAbilityEquipped);
        if (AuraASC->bStartupAbilitiesGiven)
        {
            // 콜백 함수 바인드 필요 없이 바로 호출
            BroadcastAbilityInfo();
        }
        
        // 어빌리티 부여 이전이면 델리게이트에 함수 바인딩
        if (!AuraASC->AbilitiesGivenDelegate.IsAlreadyBound(this, &UOverlayWidgetController::BroadcastAbilityInfo))
            AuraASC->AbilitiesGivenDelegate.AddDynamic(this, &UOverlayWidgetController::BroadcastAbilityInfo);
        
        AuraASC->OnMessageTagReceived.AddDynamic(this, &UOverlayWidgetController::OnMessageTagReceived);
        AuraASC->OnMessageRemoved.AddDynamic(this, &UOverlayWidgetController::MessageRemove);
        
        // 플로팅 메세지
        AuraASC->EffectAssetTags.AddDynamic(this, &UOverlayWidgetController::OnFloatingMessageReceived);
    }
    
    OnRenderTargetCreated.AddDynamic(this, &UOverlayWidgetController::MiniMapRenderTargetSet);
}

void UOverlayWidgetController::BeginDestroy()
{
    Super::BeginDestroy();
}

void UOverlayWidgetController::ProcessPendingAbilityInfos()
{
    if (PendingInfos.Num() > 0)
    {
        for (const auto& Info : PendingInfos)
        {
            AbilityInfoDelegate.Broadcast(Info);
        }
        PendingInfos.Empty();
    }
}

void UOverlayWidgetController::ShowOverlayWidget(bool bShow)
{
    if (bShow)
    {
        OnOverlayVisibilityChanged.Broadcast(true);
    }
    else
    {
        OnOverlayVisibilityChanged.Broadcast(false);
    }
}

void UOverlayWidgetController::ShowOverlayButtons(bool bShow)
{
    if (bShow)
    {
        OnButtonVisibilityChanged.Broadcast(true);
    }
    else
    {
        OnButtonVisibilityChanged.Broadcast(false);
    }
}

void UOverlayWidgetController::ShowAttributeMenuButton(bool bShow)
{
    if (bShow)
    {
        OnAttributeMenuButtonVisibilityChanged.Broadcast(true);
    }
    else
    {
        OnAttributeMenuButtonVisibilityChanged.Broadcast(false);
    }
}

void UOverlayWidgetController::ShowSpellMenuButton(bool bShow)
{
    if (bShow)
    {
        OnSpellMenuButtonVisibilityChanged.Broadcast(true);
    }
    else
    {
        OnSpellMenuButtonVisibilityChanged.Broadcast(false);
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
    if (FAuraAbilityInfo* Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag))
    {
        Info->StatusTag = Status;
        Info->InputTag = Slot;
        Info->AbilityTag = AbilityTag;
        AbilityInfoDelegate.Broadcast(*Info);
    }
}

void UOverlayWidgetController::OnMessageTagReceived(const FGameplayTag& Tag)
{
    const FUIWidgetRow* Row = GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag);
    if (Row)
        MessageWidgetRowDelegate.Broadcast(*Row);
}

void UOverlayWidgetController::OnFloatingMessageReceived(const FGameplayTagContainer& TagContainer)
{
    FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(FName("Message"));
    for (const FGameplayTag& Tag : TagContainer)
    {
        if (Tag.MatchesTag(MessageTag))
        {
            const FUIWidgetRow* Row = GetDataTableRowByTag<FUIWidgetRow>(MessageWidgetDataTable, Tag);
            MessageWidgetRowDelegate.Broadcast(*Row);
        }
    }
}

void UOverlayWidgetController::MessageRemove(const FGameplayTag& Tag)
{
    OnMessageRemoved.Broadcast(Tag);
}

void UOverlayWidgetController::MiniMapRenderTargetSet(UTextureRenderTarget2D* RenderTarget)
{
    MiniMapRenderTarget = RenderTarget;
}

void UOverlayWidgetController::SetXPBarPercentToOwnValue()
{
    if (AuraPlayerState)
    {
        OnXPChanged(AuraPlayerState->GetXP());
    }
}

void UOverlayWidgetController::RemoveCenterDescriptionMessage()
{
    OnCenterDescriptionRemoved.Broadcast();
}
