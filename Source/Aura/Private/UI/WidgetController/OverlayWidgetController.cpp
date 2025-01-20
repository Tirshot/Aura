// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "Player/AuraPlayerState.h"

void UOverlayWidgetController::BroadcastInitialValues()
{
    // 값을 어떻게 가져올까?
    const UAuraAttributeSet* AuraAttributeSet = CastChecked<UAuraAttributeSet>(AttributeSet);

    OnHealthChanged.Broadcast(AuraAttributeSet->GetHealth());
    OnMaxHealthChanged.Broadcast(AuraAttributeSet->GetMaxHealth());

    OnManaChanged.Broadcast(AuraAttributeSet->GetMana());
    OnMaxManaChanged.Broadcast(AuraAttributeSet->GetMaxMana());
}

void UOverlayWidgetController::BindCallbacksToDependencies()
{
    AAuraPlayerState* AuraPlayerState = CastChecked<AAuraPlayerState>(PlayerState);
    AuraPlayerState->OnXPChangedDelegate.AddUObject(this, &UOverlayWidgetController::OnXPChanged);
    AuraPlayerState->OnLevelChangedDelegate.AddLambda([this](int32 NewLevel)
        {
            OnPlayerLevelChangedDelegate.Broadcast(NewLevel);
        });

    const UAuraAttributeSet* AuraAttributeSet = CastChecked<UAuraAttributeSet>(AttributeSet);

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        AuraAttributeSet->GetHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnHealthChanged.Broadcast(Data.NewValue);
            }
        );

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        AuraAttributeSet->GetMaxHealthAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxHealthChanged.Broadcast(Data.NewValue);
            }
        );

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        AuraAttributeSet->GetManaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnManaChanged.Broadcast(Data.NewValue);
            }
        );

    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
        AuraAttributeSet->GetMaxManaAttribute()).AddLambda(
            [this](const FOnAttributeChangeData& Data)
            {
                OnMaxManaChanged.Broadcast(Data.NewValue);
            }
        );

    if (auto* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent))
    {
        if (AuraASC->bStartupAbilitiesGiven)
        {
            // 콜백 함수 바인드 필요 없이 바로 호출
            OnInitializeStartupAbilities(AuraASC);
        }
        else
        {
            // 어빌리티 부여 이전이면 델리게이트에 함수 바인딩
            AuraASC->AbilitiesGivenDelegate.AddUObject(this, &UOverlayWidgetController::OnInitializeStartupAbilities);
        }

        // 플로팅 메세지
        AuraASC->EffectAssetTags.AddLambda(
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

void UOverlayWidgetController::OnInitializeStartupAbilities(UAuraAbilitySystemComponent* AuraAbilitySystemComponent)
{
    // TODO : 주어진 어빌리티에 대한 모든 어빌리티 정보를 가져옴, 위젯으로 전달
    
    // 어빌리티가 주어지지 않았다면 리턴
    if (!AuraAbilitySystemComponent->bStartupAbilitiesGiven)
        return;

    // 어빌리티를 순회하지 않고 델리게이트를 사용
    // 콜백 함수 바인딩
    FForEachAbility BroadcastDelegate;
    BroadcastDelegate.BindLambda([this, AuraAbilitySystemComponent](const FGameplayAbilitySpec& AbilitySpec)
        {
            // TODO : 태그를 이용해서 어빌리티 정보 가져오기
            FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AuraAbilitySystemComponent->GetAbilityTagFromSpec(AbilitySpec));
            Info.InputTag = AuraAbilitySystemComponent->GetInputTagFromSpec(AbilitySpec);
            
            // 블루프린트 델리게이트
            AbilityInfoDelegate.Broadcast(Info);
        });

    // 델리게이트 실행
    AuraAbilitySystemComponent->ForEachAbility(BroadcastDelegate);
}

void UOverlayWidgetController::OnXPChanged(int32 NewXP) const
{
    // HUD의 XP Bar에 적용
    const AAuraPlayerState* AuraPlayerState = CastChecked<AAuraPlayerState>(PlayerState);
    
    // 레벨업 정보 가져옴
    const ULevelUpInfo* LevelUpInfo = AuraPlayerState->LevelUpInfo;

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
