// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAttributeSet.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/PlayerInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerController.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AuraAbilityTypes.h"


UAuraAttributeSet::UAuraAttributeSet()
{
    const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
    
    // 주 속성
    TagsToAttributes.Add(GameplayTags.Attributes_Primary_Strength, GetStrengthAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Primary_Intelligence, GetIntelligenceAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Primary_Resilience, GetResilienceAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Primary_Vigor, GetVigorAttribute);

    // 2차 속성
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_Armor, GetArmorAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ArmorPenetration, GetArmorPenetrationAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_BlockChance, GetBlockChanceAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitChance, GetCriticalHitChanceAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitDamage, GetCriticalHitDamageAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_CriticalHitResistance, GetCriticalHitResistanceAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_HealthRegeneration, GetHealthRegenerationAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_ManaRegeneration, GetManaRegenerationAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxHealth, GetMaxHealthAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Secondary_MaxMana, GetMaxManaAttribute);

    // 속성 저항
    TagsToAttributes.Add(GameplayTags.Attributes_Resistance_Fire, GetFireResistanceAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Resistance_Lightning, GetLightningResistanceAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Resistance_Arcane, GetArcaneResistanceAttribute);
    TagsToAttributes.Add(GameplayTags.Attributes_Resistance_Physical, GetPhysicalResistanceAttribute);

}


void UAuraAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const 
{
    // 복제할 때 반드시 오버라이드하여 사용해야 함
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // 조건 노티파이 - 복제된 속성에 사용, 조건 없음, 항상 복제(REPNOTIFY_OnChanged : 변화할 때만 복제)
    // Primary 속성
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Strength, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Resilience, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Vigor, COND_None, REPNOTIFY_Always);

    // Vital 속성
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Mana, COND_None, REPNOTIFY_Always);

    // Secondary 속성
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, Armor, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ArmorPenetration, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, BlockChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, CriticalHitResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, HealthRegeneration, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ManaRegeneration, COND_None, REPNOTIFY_Always);

    // 속성 저항
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, FireResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, LightningResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, ArcaneResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UAuraAttributeSet, PhysicalResistance, COND_None, REPNOTIFY_Always);
}

void UAuraAttributeSet::PreAttributeChange(const FGameplayAttribute & Attribute, float & NewValue)
{
    // 속성 값이 변화하기 전에 발동하는 함수
    // 클램핑 용으로만 사용할 것
    Super::PreAttributeChange(Attribute, NewValue);

    // 인수로 넘겨받은 Attribute가 해당 속성과 동일한지 확인하기
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
    }
    if (Attribute == GetManaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
    }
}

void UAuraAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData & Data)
{
    // 게임플레이 이펙트 효과가 실행된 이후에 발동되는 함수
    Super::PostGameplayEffectExecute(Data);

    // 소스가 캐릭터가 아닐 경우에 컨트롤러를 얻지 못할 수 있음!!
    FEffectProperties Props;
    SetEffectProperties(Data, Props);

    // 캐릭터 사망 이후 이펙트 실행 방지
    if (Props.TargetCharacter->Implements<UCombatInterface>())
    {
        if (ICombatInterface::Execute_IsDead(Props.TargetCharacter))
            return;
    }

    if (Data.EvaluatedData.Attribute == GetHealthAttribute())
    {
        SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
    }
    if (Data.EvaluatedData.Attribute == GetManaAttribute())
    {
        SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
    }

    // 데미지 판단
    if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
    {
        HandleIncomingDamage(Props);
    }

    // 경험치 계산
    if (Data.EvaluatedData.Attribute == GetIncomingXPAttribute())
    {
        HandleIncomingXP(Props);
    }
}

void UAuraAttributeSet::HandleIncomingDamage(const FEffectProperties& Props)
{
    const float LocalIncomingDamage = GetIncomingDamage();
    SetIncomingDamage(0);

    if (LocalIncomingDamage > 0.f)
    {
        const float NewHealth = GetHealth() - LocalIncomingDamage;
        SetHealth(FMath::Clamp(NewHealth, 0, GetMaxHealth()));

        const bool bFatal = NewHealth <= 0.f;
        if (bFatal)
        {
            ICombatInterface* CombatInterface = Cast<ICombatInterface>(Props.TargetAvatarActor);
            if (CombatInterface)
            {
                // 사망
                CombatInterface->Die(UAuraAbilitySystemLibrary::GetDeathImpulse(Props.EffectContextHandle));
            }
            SendXPEvent(Props);
        }
        else
        {
            // 캐릭터 대상으로 감전사 타격중이 아니라면
            if (Props.TargetCharacter->Implements<UCombatInterface>() && !ICombatInterface::Execute_IsBeingShocked(Props.TargetCharacter))
            {
                // HitReact 태그 부여
                FGameplayTagContainer TagContainer;
                TagContainer.AddTag(FAuraGameplayTags::Get().Effects_HitReact);

                // 부여된 어빌리티를 찾아서 활성화
                Props.TargetASC->TryActivateAbilitiesByTag(TagContainer);
            }

            // 넉백 적용
            const FVector& KnockbackForce = UAuraAbilitySystemLibrary::GetKnockbackForce(Props.EffectContextHandle);
            if (KnockbackForce.IsNearlyZero(10.f) == false)
            {
                Props.TargetCharacter->LaunchCharacter(KnockbackForce, true, true);
            }
        }

        const bool bBlock = UAuraAbilitySystemLibrary::IsBlockedHit(Props.EffectContextHandle);
        const bool bCriticalHit = UAuraAbilitySystemLibrary::IsCriticalHit(Props.EffectContextHandle);
        ShowFloatingText(Props, LocalIncomingDamage, bBlock, bCriticalHit);
        if (UAuraAbilitySystemLibrary::IsSuccessfulDebuff(Props.EffectContextHandle))
        {
            // 디버프 적용
            Debuff(Props);
        }
    }
}

void UAuraAttributeSet::Debuff(const FEffectProperties& Props)
{
    const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
    // 게임플레이 이펙트 컨텍스트 핸들 생성 및 정보 추가
    FGameplayEffectContextHandle EffectContext = Props.SourceASC->MakeEffectContext();
    EffectContext.AddSourceObject(Props.SourceAvatarActor);

    // 디버프 정보 가져오기
    const FGameplayTag DamageType = UAuraAbilitySystemLibrary::GetDamageType(Props.EffectContextHandle);
    const float DebuffDamage = UAuraAbilitySystemLibrary::GetDebuffDamage(Props.EffectContextHandle);
    const float DebuffDuration = UAuraAbilitySystemLibrary::GetDebuffDuration(Props.EffectContextHandle);
    const float DebuffFrequency = UAuraAbilitySystemLibrary::GetDebuffFrequency(Props.EffectContextHandle);

    FString DebuffName = FString::Printf(TEXT("Dynamic Debuff_%s"), *DamageType.ToString());

    // 게임플레이 이펙트 생성
    UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(DebuffName));
    
    // 이펙트 설정
    Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;  // 지속형
    Effect->Period = DebuffFrequency;                                                   // 주기
    Effect->DurationMagnitude = FScalableFloat(DebuffDuration);             // 지속 시간

    // 데미지 타입에 따른 디버프의 태그 가져오기
    const FGameplayTag DebuffTag = GameplayTags.DamageTypesToDebuff[DamageType];
    Effect->InheritableOwnedTagsContainer.AddTag(DebuffTag);
    
    // 스턴 상태
    if (DebuffTag.MatchesTagExact(GameplayTags.Debuff_Stun))
    {
        Effect->InheritableOwnedTagsContainer.AddTag(GameplayTags.Player_Block_CursorTrace);
        Effect->InheritableOwnedTagsContainer.AddTag(GameplayTags.Player_Block_InputHeld);
        Effect->InheritableOwnedTagsContainer.AddTag(GameplayTags.Player_Block_InputPressed);
        Effect->InheritableOwnedTagsContainer.AddTag(GameplayTags.Player_Block_InputReleased);
    }

    // 디버프 스택 설정
    Effect->StackingType = EGameplayEffectStackingType::AggregateBySource;
    Effect->StackLimitCount = 1;

    // 수정자 추가
    const int32 Index = Effect->Modifiers.Num();
    Effect->Modifiers.Add(FGameplayModifierInfo());
    FGameplayModifierInfo& ModifierInfo = Effect->Modifiers[Index];

    // 수정자 설정
    ModifierInfo.ModifierMagnitude = FScalableFloat(DebuffDamage);
    ModifierInfo.ModifierOp = EGameplayModOp::Additive;
    ModifierInfo.Attribute = UAuraAttributeSet::GetIncomingDamageAttribute();

    // 게임플레이 스펙 생성
    FGameplayEffectSpec* MutableSpec = new FGameplayEffectSpec(Effect, EffectContext, 1.f/*Debuff Level*/);
    if (MutableSpec)
    {
        FAuraGameplayEffectContext* AuraContext = static_cast<FAuraGameplayEffectContext*>(EffectContext.Get());
        TSharedPtr<FGameplayTag> DebuffDamageType = MakeShareable(new FGameplayTag(DamageType));
        AuraContext->SetDamageType(DebuffDamageType);
    }
    
    // 게임플레이 이펙트 적용
    Props.TargetASC->ApplyGameplayEffectSpecToSelf(*MutableSpec);
}

void UAuraAttributeSet::HandleIncomingXP(const FEffectProperties& Props)
{
    const float LocalIncomingXP = GetIncomingXP();
    SetIncomingXP(0);

    // TODO : 레벨업 체크 필요

    // Source Character는 보유자, GA_ListenForEvents가 GE_EventBasedEffect를 사용하여 IncomingXP를 더하는 중
    if (Props.SourceCharacter->Implements<UPlayerInterface>() && Props.SourceCharacter->Implements<UCombatInterface>())
    {
        const int32 CurrentLevel = ICombatInterface::Execute_GetCharacterLevel(Props.SourceCharacter);
        const int32 CurrentXP = IPlayerInterface::Execute_GetXP(Props.SourceCharacter);

        // 현재 경험치에 새 경험치를 더한 레벨 계산
        const int32 NewLevel = IPlayerInterface::Execute_FindLevelForXP(Props.SourceCharacter, CurrentXP + LocalIncomingXP);

        // 레벨업 몇 번
        const int32 NumLevelUps = NewLevel - CurrentLevel;

        // 레벨업 숫자 체크
        if (NumLevelUps > 0)
        {
            // 레벨업 보상
            const int32 AttributePointsReward = IPlayerInterface::Execute_GetAttributePointsReward(Props.SourceCharacter, CurrentLevel);
            const int32 SpellPointsReward = IPlayerInterface::Execute_GetSpellPointsReward(Props.SourceCharacter, CurrentLevel);

            // 레벨 더하기
            IPlayerInterface::Execute_AddToPlayerLevel(Props.SourceCharacter, NumLevelUps);
            IPlayerInterface::Execute_AddToAttributePoints(Props.SourceCharacter, AttributePointsReward);
            IPlayerInterface::Execute_AddToSpellPoints(Props.SourceCharacter, SpellPointsReward);

            // 체력과 마나 회복하기
            bTopOffHealth = true;
            bTopOffMana = true;

            IPlayerInterface::Execute_LevelUp(Props.SourceCharacter);
        }

        IPlayerInterface::Execute_AddToXP(Props.SourceCharacter, LocalIncomingXP);
    }
}

void UAuraAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    // 레벨업 체크 불리언
    if (Attribute == GetMaxHealthAttribute() && bTopOffHealth)
    {
        SetHealth(GetMaxHealth());
        bTopOffHealth = false;
    }

    if (Attribute == GetMaxManaAttribute() && bTopOffMana)
    {
        SetMana(GetMaxMana());
        bTopOffMana = false;
    }
}


void UAuraAttributeSet::ShowFloatingText(const FEffectProperties& Props, float Damage, bool bBlockedHit, bool bCriticalHit) const
{
    // 데미지 플로팅 위젯
    // 자해 불가
    if (Props.SourceCharacter != Props.TargetCharacter)
    {
        if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Props.SourceController))
        {
            PC->ShowDamageNumber(Damage, Props.TargetCharacter, bBlockedHit, bCriticalHit);
            return;
        }
        
        // 맞는 사람이 플레이어 캐릭터일 때
        if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Props.TargetCharacter->Controller))
        {
            PC->ShowDamageNumber(Damage, Props.TargetCharacter, bBlockedHit, bCriticalHit);
        }
    }
}

void UAuraAttributeSet::SendXPEvent(const FEffectProperties& Props)
{
    // 대상의 클래스와 레벨 가져옴
    if (Props.TargetCharacter->Implements<UCombatInterface>())
    {
        // XP 보상 계산
        int32 TargetLevel = ICombatInterface::Execute_GetCharacterLevel(Props.TargetCharacter);
        ECharacterClass TargetClass = ICombatInterface::Execute_GetCharacterClass(Props.TargetCharacter);

        const int32 XPReward = UAuraAbilitySystemLibrary::GetXPRewardForClassAndLevel(Props.TargetCharacter, TargetClass, TargetLevel);

        // 이벤트 전달
        const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
        
        // 페이로드 수정
        FGameplayEventData Payload;
        Payload.EventTag = GameplayTags.Attributes_Meta_IncomingXP;
        Payload.EventMagnitude = XPReward;

        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Props.SourceCharacter, GameplayTags.Attributes_Meta_IncomingXP, Payload);
    }
}

void UAuraAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Health, OldHealth);
}

void UAuraAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Mana, OldMana);
}

void UAuraAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxHealth, OldMaxHealth);
}

void UAuraAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, MaxMana, OldMaxMana);
}

void UAuraAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldStrength) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Strength, OldStrength);
}

void UAuraAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldIntelligence) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Intelligence, OldIntelligence);
}

void UAuraAttributeSet::OnRep_Resilience(const FGameplayAttributeData& OldResilience) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Resilience, OldResilience);
}

void UAuraAttributeSet::OnRep_Vigor(const FGameplayAttributeData& OldVigor) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Vigor, OldVigor);
}

void UAuraAttributeSet::OnRep_Armor(const FGameplayAttributeData& OldArmor) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, Armor, OldArmor);
}

void UAuraAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldArmorPenetration) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ArmorPenetration, OldArmorPenetration);
}

void UAuraAttributeSet::OnRep_BlockChance(const FGameplayAttributeData& OldBlockChance) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, BlockChance, OldBlockChance);
}

void UAuraAttributeSet::OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitChance, OldCriticalHitChance);
}

void UAuraAttributeSet::OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitDamage, OldCriticalHitDamage);
}

void UAuraAttributeSet::OnRep_CriticalHitResistance(const FGameplayAttributeData& OldCriticalHitResistance) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, CriticalHitResistance, OldCriticalHitResistance);
}

void UAuraAttributeSet::OnRep_HealthRegeneration(const FGameplayAttributeData& OldHealthRegeneration) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, HealthRegeneration, OldHealthRegeneration);
}

void UAuraAttributeSet::OnRep_ManaRegeneration(const FGameplayAttributeData& OldManaRegeneration) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ManaRegeneration, OldManaRegeneration);
}

void UAuraAttributeSet::OnRep_FireResistance(const FGameplayAttributeData& OldFireResistance) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, FireResistance, OldFireResistance);
}

void UAuraAttributeSet::OnRep_LightningResistance(const FGameplayAttributeData& OldLightningResistance) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, LightningResistance, OldLightningResistance);
}

void UAuraAttributeSet::OnRep_ArcaneResistance(const FGameplayAttributeData& OldArcaneResistance) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, ArcaneResistance, OldArcaneResistance);
}

void UAuraAttributeSet::OnRep_PhysicalResistance(const FGameplayAttributeData& OldPhysicalResistance) const
{
    // Notify 매크로
    GAMEPLAYATTRIBUTE_REPNOTIFY(UAuraAttributeSet, PhysicalResistance, OldPhysicalResistance);
}

void UAuraAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData & Data, FEffectProperties& Props) const
{
    // 게임플레이 이펙트가 속성을 변경한 후에 실행(PreAttributeChange -> 속성 변경 -> 이 함수)
    Props.EffectContextHandle = Data.EffectSpec.GetContext();

    // 소스 - 이펙트 발동자, 타겟 - 이펙트의 대상(속성 세트의 주인)
    Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

    if (Props.SourceASC && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
    {
        Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
        Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
        if (Props.SourceController == nullptr && Props.SourceAvatarActor != nullptr)
        {
            // 폰에서 플레이어 컨트롤러 가져오기
            if (const APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
            {
                Props.SourceController = Pawn->GetController();
            }
        }
        if (Props.SourceController)
        {
            Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
        }
    }

    // 타겟의 어빌리티 시스템 컴포넌트에 접근하기
    if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
    {
        Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
        Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
        Props.TargetCharacter = Cast<ACharacter>(Props.TargetAvatarActor);
        Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
    }
}