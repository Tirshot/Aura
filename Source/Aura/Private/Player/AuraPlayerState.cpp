// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerState.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/Data/LevelUpInfo.h"


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

void AAuraPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAuraPlayerState, Level);
    DOREPLIFETIME(AAuraPlayerState, XP);
    DOREPLIFETIME(AAuraPlayerState, AttributePoints);
    DOREPLIFETIME(AAuraPlayerState, SpellPoints);
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
    OnLevelChangedDelegate.Broadcast(Level);
}

void AAuraPlayerState::AddToLevel(int32 InLevel)
{
    Level += InLevel;
    OnLevelChangedDelegate.Broadcast(Level);
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

void AAuraPlayerState::OnRep_Level(int32 OldLevel)
{
    // 블루프린트로 전달
    OnLevelChangedDelegate.Broadcast(Level);
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
