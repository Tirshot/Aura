// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraGameplayAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
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

FString UAuraGameplayAbility::GetLockedDescription(int32 Level, int32 InferiorAbilityLevel)
{
    if (InferiorAbilityLevel > 0)
        return FString::Printf(TEXT("<Default>캐릭터 레벨 </><Level>%d</><Default>부터 습득 가능.</>\n<Default>하위 스펠 레벨 </><Level>%d</><Default>부터 습득 가능.</>"), Level, InferiorAbilityLevel);
    
    return FString::Printf(TEXT("<Default>캐릭터 레벨 </><Level>%d</><Default>부터 습득 가능.</>"), Level);
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
    
    if (AActor* OwnerActor = AvatarActor->GetOwner())
    {
        if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(OwnerActor))
        {
            if (AAuraPlayerState* AuraPlayerState = AuraPC->GetPlayerState<AAuraPlayerState>())
            {
                return AuraPlayerState->OwnedAbilityUpgradeTags.HasTag(Tag);
            }
        }
    }
    return false;
}

int32 UAuraGameplayAbility::GetUpgradeStackCount(AActor* AvatarActor,FGameplayTag Tag)
{
    if (!Tag.IsValid() || !AvatarActor)
        return 0;

    if (APlayerController* PC = Cast<APlayerController>(AvatarActor->GetOwner()))
    {
        if (AAuraPlayerState* AuraPS = PC->GetPlayerState<AAuraPlayerState>())
        {
            return AuraPS->GetUpgradeTagCount(Tag);
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
