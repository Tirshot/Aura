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

void UAuraWidgetController::BroadcastAbilityInfo()
{
    // 어빌리티가 주어지지 않았다면 리턴
    if (!GetAuraASC()->bStartupAbilitiesGiven)
        return;

    // 어빌리티를 순회하지 않고 델리게이트를 사용
    // 콜백 함수 바인딩
    FForEachAbility BroadcastDelegate;
    BroadcastDelegate.BindLambda([this](const FGameplayAbilitySpec& AbilitySpec)
        {
            // 태그를 이용해서 어빌리티 정보 가져오기
            FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(GetAuraASC()->GetAbilityTagFromSpec(AbilitySpec));

            // 어빌리티에 입력 태그 및 상태 태그 부여
            Info.InputTag = GetAuraASC()->GetInputTagFromSpec(AbilitySpec);
            Info.StatusTag = GetAuraASC()->GetStatusFromSpec(AbilitySpec);

            // 블루프린트 델리게이트로 전송
            AbilityInfoDelegate.Broadcast(Info);
        });

    // 델리게이트 실행
    GetAuraASC()->ForEachAbility(BroadcastDelegate);
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
