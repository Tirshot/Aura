// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/AuraWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityInfo.h"

void UAuraWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WCParams)
{
    PlayerController = WCParams.PlayerController;
    PlayerState = WCParams.PlayerState;
    AbilitySystemComponent = WCParams.AbilitySystemComponent;
    AttributeSet = WCParams.AttributeSet;
}

void UAuraWidgetController::BroadcastDelegates(const FGameplayAbilitySpec& AbilitySpec)
{
    FAuraAbilityInfo* Info = AbilityInfo->FindAbilityInfoForTag(GetAuraASC()->GetAbilityTagFromSpec(AbilitySpec));

    Info->InputTag = GetAuraASC()->GetInputTagFromSpec(AbilitySpec);
    Info->StatusTag = GetAuraASC()->GetStatusFromSpec(AbilitySpec);

    if (AbilityInfoDelegate.IsBound())
    {
        AbilityInfoDelegate.Broadcast(*Info);
    }
    else
    {
        // 바인딩이 더 늦으면 캐싱해뒀다가 다시 뿌려주기
        PendingInfos.Add(*Info);
    }
}

void UAuraWidgetController::BroadcastAbilityInfo()
{
    if (!GetAuraASC()->bStartupAbilitiesGiven)
        return;

    FForEachAbility BroadcastDelegate;
    BroadcastDelegate.BindUObject(this, &UAuraWidgetController::BroadcastDelegates);

    GetAuraASC()->ForEachAbility(BroadcastDelegate);
}

void UAuraWidgetController::SetAttributeSet(UAttributeSet* AS)
{
    AttributeSet = AS;
    AuraAttributeSet = Cast<UAuraAttributeSet>(AS);
}

AAuraPlayerController* UAuraWidgetController::GetAuraPC()
{
    if (AuraPlayerController == nullptr)
    {
        AuraPlayerController = Cast<AAuraPlayerController>(PlayerController);
    }
    return AuraPlayerController;
}

AAuraPlayerState* UAuraWidgetController::GetAuraPS()
{
    if (AuraPlayerState == nullptr)
    {
        AuraPlayerState = Cast<AAuraPlayerState>(PlayerState);
    }
    return AuraPlayerState;
}

UAuraAbilitySystemComponent* UAuraWidgetController::GetAuraASC()
{
    if (AuraAbilitySystemComponent == nullptr)
    {
        AuraAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent);
    }
    return AuraAbilitySystemComponent;
}

UAuraAttributeSet* UAuraWidgetController::GetAuraAS()
{
    if (AuraAttributeSet == nullptr)
    {
        AuraAttributeSet = Cast<UAuraAttributeSet>(AttributeSet);
    }
    return AuraAttributeSet;
}
