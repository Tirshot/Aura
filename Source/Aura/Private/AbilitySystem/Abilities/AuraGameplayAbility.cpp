// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Character/AuraEnemy.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"

FString UAuraGameplayAbility::GetDescription(int32 Level, const UObject* WorldContextObject)
{
    return FString::Printf(TEXT("<Default>%s, </><Level>%d</>"), L"Default Ability Name - LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum", Level);
}

FString UAuraGameplayAbility::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
    return FString::Printf(TEXT("<Default>%s, </><Level>%d</>"), L"Default Ability Name - LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum LoremIpsum", Level);
}

FString UAuraGameplayAbility::GetLockedDescription(int32 Level, const UObject* WorldContextObject, const FGameplayTag& InferiorAbilityTag)
{
    // 빈 태그
    if (InferiorAbilityTag.MatchesTag(FGameplayTag::RequestGameplayTag("Abilities.None"))
        || InferiorAbilityTag.MatchesTag(FGameplayTag::EmptyTag))
        return FString::Printf(TEXT("<Default>캐릭터 레벨 </><Level>%d</><Default>부터 습득 가능합니다.</>"), Level);

    // 하위 어빌리티 태그 받음
    {
        auto AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(WorldContextObject);
        FName InferiorAbilityName = AbilityInfo->GetAbilityNameForTag(InferiorAbilityTag);
        
        return FString::Printf(TEXT("<Default>캐릭터 레벨 </><Level>%d</><Default>부터 습득 가능합니다.</>\n<Damage>하위 어빌리티 %s를 습득해야 합니다.</>")
            ,Level
            ,*InferiorAbilityName.ToString());
    }
}

TArray<FAuraAbilityUpgradeInfo> UAuraGameplayAbility::GetAbilityUpgradeForTag(AActor* AvatarActor, FGameplayTag AbilityTag)
{
    auto* AllUpgradeInfos = UAuraAbilitySystemLibrary::GetAbilityUpgradeInfo(AvatarActor);
    if (AllUpgradeInfos == nullptr)
        return TArray<FAuraAbilityUpgradeInfo>();

    auto UpgradeArray = AllUpgradeInfos->GetUpgradesForAbility(AbilityTag);
    TArray<FAuraAbilityUpgradeInfo> AbilityUpgradeInfos = UpgradeArray.UpgradeInfos;
    
    if (AbilityUpgradeInfos.IsEmpty())
        return TArray<FAuraAbilityUpgradeInfo>();

    return AbilityUpgradeInfos;
}

bool UAuraGameplayAbility::HasUpgradeTag(AActor* AvatarActor, FGameplayTag Tag)
{
    if (!AvatarActor)
        return false;

    // 플레이어 캐릭터
    if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(Cast<APawn>(AvatarActor)->GetController()))
    {
        if (AAuraPlayerState* AuraPlayerState = AuraPC->GetPlayerState<AAuraPlayerState>())
        {
            return AuraPlayerState->HasUpgradeTag(Tag);
        }
    }

    // 몬스터 캐릭터
    if (AAuraEnemy* Enemy = Cast<AAuraEnemy>(AvatarActor))
    {
        return Enemy->GetAbilitySystemComponent()->HasMatchingGameplayTag(Tag);
    }
    
    return false;
}

int32 UAuraGameplayAbility::GetUpgradeStackCount(AActor* AvatarActor,FGameplayTag Tag)
{
    if (!Tag.IsValid() || !AvatarActor)
        return 0;

    // 플레이어 캐릭터
    if (APlayerController* PC = Cast<APlayerController>(Cast<APawn>(AvatarActor)->GetController()))
    {
        if (AAuraPlayerState* AuraPS = PC->GetPlayerState<AAuraPlayerState>())
        {
            return AuraPS->GetUpgradeTagCount(Tag);
        }
    }

    // 몬스터 캐릭터
    if (AAuraEnemy* Enemy = Cast<AAuraEnemy>(AvatarActor))
    {
        if (Enemy->GetAbilitySystemComponent()->HasMatchingGameplayTag(Tag))
        {
            return 1;
        }
    }
    
    return 0;
}

float UAuraGameplayAbility::GetManaCost(float InLevel) const
{
    float ManaCost = 0.f;

    // �ڽ�Ʈ �����÷��� ����Ʈ
    if (const UGameplayEffect* CostEffect = GetCostGameplayEffect())
    {
        for (FGameplayModifierInfo Mod : CostEffect->Modifiers)
        {
            // �ڽ�Ʈ�� ����
            if (Mod.Attribute == UAuraAttributeSet::GetManaAttribute())
            {
                Mod.ModifierMagnitude.GetStaticMagnitudeIfPossible(InLevel, ManaCost);
                break;
            }
        }
    }
    return ManaCost;
}

float UAuraGameplayAbility::GetCoolDown(float InLevel) const
{
    float Cooldown = 0.f;
    if (const UGameplayEffect* CooldownEffect = GetCooldownGameplayEffect())
    {
        // ���� �����÷��� ����Ʈ
        CooldownEffect->DurationMagnitude.GetStaticMagnitudeIfPossible(InLevel, Cooldown);
    }
    return Cooldown;
}
