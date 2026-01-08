// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraLogChannels.h"
#include "Interaction/PlayerInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Game/LoadScreenSaveGame.h"
#include "Interaction/CombatInterface.h"

void UAuraAbilitySystemComponent::AbilityActorInfoSet()
{
    OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAuraAbilitySystemComponent::ClientEffectApplied);
}

void UAuraAbilitySystemComponent::AddCharacterAbilitiesFromSaveData(ULoadScreenSaveGame* SaveData)
{
    // 저장된 어빌리티 순회
    for (const FSavedAbility& Data : SaveData->SavedAbilities)
    {
        const TSubclassOf<UGameplayAbility> LoadedAbilityClass = Data.GameplayAbility;

        FGameplayAbilitySpec LoadedAbilitySpec = FGameplayAbilitySpec(LoadedAbilityClass, Data.AbilityLevel);
        LoadedAbilitySpec.GetDynamicSpecSourceTags().AddTag(Data.AbilitySlot);
        LoadedAbilitySpec.GetDynamicSpecSourceTags().AddTag(Data.AbilityStatus);
        
        if (Data.AbilityType == FAuraGameplayTags::Get().Abilities_Type_Offensive)
        {
            GiveAbility(LoadedAbilitySpec);
        }
        else if (Data.AbilityType == FAuraGameplayTags::Get().Abilities_Type_Passive)
        {
            if (Data.AbilityStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped)
                || Data.AbilityTag.MatchesTag(FGameplayTag::RequestGameplayTag("Abilities.Passive.ListenForEvent")))
            {
                GiveAbility(LoadedAbilitySpec);
                GiveAbilityAndActivateOnce(LoadedAbilitySpec);
            }
        }
    }
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
    UpdateAbilityStatus(SaveData->PlayerLevel);
}

void UAuraAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
    UAuraGameplayAbility* AuraAbility = nullptr;
    FGameplayTag StartupInputTag;
    
    for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
    {
        // 게임플레이 어빌리티 스펙 생성
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
        AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability);
        
        // 어빌리티에 입력 태그를 부여
        if (AuraAbility)
        {
            StartupInputTag = AuraAbility->StartupInputTag;
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupInputTag);
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
            
            GiveAbility(AbilitySpec);

            // 시작 태그가 동일할 경우 덮어씌우기
            FScopedAbilityListLock ActiveScopeLoc(*this);
            for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
            {
                if (Spec.Ability == AbilitySpec.Ability)
                    continue;
        
                if (Spec.GetDynamicSpecSourceTags().HasTag(StartupInputTag))
                {
                    // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                    Spec.GetDynamicSpecSourceTags().RemoveTag(StartupInputTag);
                    Spec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                    Spec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                }
            }
        }
    }
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
    UpdateAbilityStatus(1);
}

void UAuraAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
    UAuraGameplayAbility* AuraAbility = nullptr;
    FGameplayTag StartupInputTag;
    
    for (TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
    {
        // 게임플레이 어빌리티 스펙 생성
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
        AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability);
        
        if (AuraAbility)
        {
            StartupInputTag = AuraAbility->StartupInputTag;
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupInputTag);
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
        }
        // GA_ListenForEvent는 오라 어빌리티가 아님!!
        GiveAbility(AbilitySpec);
        GiveAbilityAndActivateOnce(AbilitySpec);
        
        // 패시브 이펙트 활성화
        MulticastActivatePassiveEffect(GetAbilityTagFromSpec(AbilitySpec), true);

        // 시작 태그가 동일할 경우 덮어씌우기
        FScopedAbilityListLock ActiveScopeLoc(*this);
        for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
        {
            if (Spec.Ability == AbilitySpec.Ability || AbilitySpec.Ability == nullptr)
                continue;
        
            if (Spec.GetDynamicSpecSourceTags().HasTag(StartupInputTag))
            {
                // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                // 패시브 비활성화
                Spec.GetDynamicSpecSourceTags().RemoveTag(StartupInputTag);
                Spec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                Spec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                DeactivePassiveAbility.Broadcast(GetAbilityTagFromSpec(Spec));
            }
        }
    }
    AbilitiesGivenDelegate.Broadcast();
    UpdateAbilityStatus(1);
}

void UAuraAbilitySystemComponent::AddCharacterAbilityByTag(const FGameplayTag& AbilityTag)
{
    // 게임플레이 어빌리티 스펙 생성
    if (UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this))
    {
        if (FAuraAbilityInfo* AuraAbilityInfo = AbilityInfo->FindAbilityInfoForTag(AbilityTag))
        {
            if (auto Ability = AuraAbilityInfo->Ability)
            {
                int AbilityLevel = GetAbilityLevelByTag(AbilityTag);
        
                // 이미 가지고 있는 어빌리티
                if (AbilityLevel > 0)
                {
                    FGameplayTag StartupInputTag;
                    UAuraGameplayAbility* AuraAbility = nullptr;
            
                    FScopedAbilityListLock ActiveScopeLoc(*this);
                    for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
                    {
                        // 활성화 된 모든 어빌리티 스펙에서 태그와 일치하는 스펙 찾기
                        if (AbilitySpec.Ability->GetAssetTags().HasTagExact(AbilityTag))
                        {
                            if (AbilitySpec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Eligible))
                            {
                                // 해금 된 어빌리티
                                AbilitySpec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Eligible);
                                AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                                AbilitySpec.Level = 1;

                                // 초기 입력 태그 설정
                                AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability);
                        
                                if (AuraAbility)
                                {
                                    StartupInputTag = AuraAbility->StartupInputTag;
                                    AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupInputTag);
                                }
                            }
                            else if (AbilitySpec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked)
                                || AbilitySpec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Equipped))
                            {
                                // 이미 익힌 어빌리티 또는 장착한 어빌리티
                                AbilitySpec.Level += 1;
                            }
                            // 어빌리티 레벨업
                            break;
                        }
                    }

                    // 시작 태그가 동일할 경우 덮어씌우기
                    for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
                    {
                        if (AuraAbility && AbilitySpec.GetDynamicSpecSourceTags().HasTag(StartupInputTag))
                        {
                            // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                            if (AuraAbility != AbilitySpec.Ability)
                            {
                                AbilitySpec.GetDynamicSpecSourceTags().RemoveTag(StartupInputTag);
                                AbilitySpec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                                AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                            }
                        }
                    }
                }
                else
                {
                    FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1);

                    FGameplayTag StartupInputTag;
                    UAuraGameplayAbility* AuraAbility = nullptr;
            
                    AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability);
            
                    // 어빌리티에 입력 태그를 부여
                    if (AuraAbility)
                    {
                        StartupInputTag = AuraAbility->StartupInputTag;
                        AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupInputTag);
                        AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                    }

                    bool bIsPassiveAbility = AbilityTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities.Passive")));
                    if (bIsPassiveAbility)
                    {
                        GiveAbilityAndActivateOnce(AbilitySpec);
                        MulticastActivatePassiveEffect(AbilityTag, true);
                    }
                    else
                    {
                        GiveAbility(AbilitySpec);
                    }

                    // 시작 태그가 동일할 경우 덮어씌우기
                    FScopedAbilityListLock ActiveScopeLoc(*this);
                    for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
                    {
                        if (AuraAbility && Spec.GetDynamicSpecSourceTags().HasTag(StartupInputTag))
                        {
                            // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                            if (AuraAbility != Spec.Ability)
                            {
                                AbilitySpec.GetDynamicSpecSourceTags().RemoveTag(StartupInputTag);
                                AbilitySpec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                                AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                        
                                FGameplayTag SpecTag = GetAbilityTagFromSpec(Spec);

                                // 패시브면 비활성화
                                if (SpecTag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities.Passive"))))
                                {
                                    MulticastActivatePassiveEffect(SpecTag, false);
                                    DeactivePassiveAbility.Broadcast(SpecTag);
                                }
                            }
                        }
                    }
                }
                AbilitiesGivenDelegate.Broadcast();

                if (GetAvatarActor()->Implements<UCombatInterface>())
                {
                    int32 Level = 0;
                    Level = ICombatInterface::Execute_GetCharacterLevel(GetAvatarActor());
                    UpdateAbilityStatus(Level);
                    return;
                }
        
                UpdateAbilityStatus(1);
            }
        }
    }
}

void UAuraAbilitySystemComponent::RemoveCharacterAbilityByTag(const FGameplayTag& AbilityTag, const int32 RemoveCount)
{
    if (UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this))
    {
        if (FAuraAbilityInfo* AuraAbilityInfo = AbilityInfo->FindAbilityInfoForTag(AbilityTag))
        {
            if (auto Ability = AuraAbilityInfo->Ability)
            {
                int AbilityLevel = GetAbilityLevelByTag(AbilityTag);
        
                // 이미 가지고 있는 어빌리티
                FGameplayTag InputTag;
                UAuraGameplayAbility* AuraAbility = nullptr;
            
                FScopedAbilityListLock ActiveScopeLoc(*this);
                for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
                {
                    // 활성화 된 모든 어빌리티 스펙에서 태그와 일치하는 스펙 찾기
                    if (AbilitySpec.Ability->GetAssetTags().HasTagExact(AbilityTag))
                    {
                        InputTag = GetInputTagFromSpec(AbilitySpec);
                        
                        if (AbilitySpec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked)
                            || AbilitySpec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Equipped))
                        {
                            // 이미 익힌 어빌리티 또는 장착한 어빌리티
                            AbilitySpec.Level -= RemoveCount;
                                
                            // 어빌리티 레벨 감소로 인해 재잠금
                            if (AbilitySpec.Level <= 0)
                            {
                                AbilitySpec.Level = 0;
                                AbilitySpec.GetDynamicSpecSourceTags().Reset();
                                AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Locked);
                                
                                // 장착된 슬롯 해제
                                AbilityEquipped.Broadcast(FGameplayTag::EmptyTag, FAuraGameplayTags::Get().Abilities_Status_Locked, InputTag, InputTag);
                            }
                            break;
                        }
                    }
                }
                AbilitiesGivenDelegate.Broadcast();

                if (GetAvatarActor()->Implements<UCombatInterface>())
                {
                    int32 Level = 0;
                    Level = ICombatInterface::Execute_GetCharacterLevel(GetAvatarActor());
                    UpdateAbilityStatus(Level);
                    return;
                }
        
                UpdateAbilityStatus(1);
            }
        }
    }
}

void UAuraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
    if (InputTag.IsValid() == false)
        return;

    const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

    FScopedAbilityListLock ActiveScopeLoc(*this);
    if (HasMatchingGameplayTag(GameplayTags.Player_Abilities_WaitForExecute))
    {
        for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
        {
            if (AbilitySpec.Ability.Get()->GetAssetTags().HasTag(FGameplayTag::RequestGameplayTag("Abilities.None")))
                continue;
            
            if (AbilitySpec.IsActive())
            {
                // 왼쪽 버튼 클릭 혹은 어빌리티 버튼 입력
                bool bValidInput = InputTag.MatchesTagExact(GameplayTags.InputTag_LMB)
                                || AbilitySpec.GetDynamicSpecSourceTags().HasTag(InputTag);

                // 다른 키 입력 시 어빌리티 취소
                if (bValidInput)
                {
                    AbilitySpecInputPressed(AbilitySpec);
                    
                    if (AbilitySpec.IsActive())
                    {
                        TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
                        const FGameplayAbilityActivationInfo& ActivationInfo = Instances.Last()->GetCurrentActivationInfoRef();
                        InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, ActivationInfo.GetActivationPredictionKey());
                        return;
                    }
                }
                else
                {
                    CancelAbilityHandle(AbilitySpec.Handle);
                }
            }
        }
        return;
    }
    
    for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
    {
        // 어빌리티에 할당된 입력태그가 입력으로 받은 입력태그와 일치
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
        {
            // 해당 어빌리티에 대해 누르기 발동
            AbilitySpecInputPressed(AbilitySpec);
            if (AbilitySpec.IsActive())
            {
                TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
                const FGameplayAbilityActivationInfo& ActivationInfo = Instances.Last()->GetCurrentActivationInfoRef();
                InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, ActivationInfo.GetActivationPredictionKey());
            }
        }
        return;
    }
}

void UAuraAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
    // 입력 태그가 유효하지 않으면 종료
    if (!InputTag.IsValid())
        return;

    // 어빌리티 스코프 잠금
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (auto& AbilitySpec : GetActivatableAbilities())
    {
        // 태그 컨테이너를 순회하여 입력 태그와 일치하는 어빌리티 스펙을 찾음
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag))
        {
            // 입력 확인
            AbilitySpecInputPressed(AbilitySpec);

            // 활성화 되어 있지 않으면 활성화
            if (AbilitySpec.IsActive() == false)
            {
                TryActivateAbility(AbilitySpec.Handle);
            }
        }
    }
}

void UAuraAbilitySystemComponent::AbilityInputTagReleased(const FGameplayTag& InputTag)
{
    // 버튼을 땠을 때 종료되거나, 계속 유지됨
    
    // 입력 태그가 유효하지 않으면 종료
    if (!InputTag.IsValid())
        return;

    // 어빌리티 스코프 잠금
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (auto& AbilitySpec : GetActivatableAbilities())
    {
        // 태그 컨테이너를 순회하여 입력 태그와 일치하는 어빌리티 스펙을 찾음
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(InputTag) && AbilitySpec.IsActive())
        {
            // 입력 확인
            AbilitySpecInputReleased(AbilitySpec);
            
            if (AbilitySpec.IsActive())
            {
                TArray<UGameplayAbility*> Instances = AbilitySpec.GetAbilityInstances();
                const FGameplayAbilityActivationInfo& ActivationInfo = Instances.Last()->GetCurrentActivationInfoRef();
                InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, ActivationInfo.GetActivationPredictionKey());
            }
        }
    }
}

void UAuraAbilitySystemComponent::ForEachAbility(const FForEachAbility& Delegate)
{
    // 어빌리티가 차단되거나 변경되었을 수 있음, 어빌리티 잠금
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
    {
        if (Delegate.ExecuteIfBound(AbilitySpec) == false)
        {
            UE_LOG(LogAura, Error, TEXT("Failed to execute delegate in %hs"), __FUNCTION__);
        }
    }
}

FGameplayTag UAuraAbilitySystemComponent::GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
    for (FGameplayTag Tag : AbilitySpec.GetDynamicSpecSourceTags())
    {
        if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("InputTag"))))
        {
            return Tag;
        }
    }
    return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
    for (FGameplayTag Tag : AbilitySpec.GetDynamicSpecSourceTags())
    {
        if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Status"))))
        {
            return Tag;
        }
    }
    return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromSpec(FGameplayAbilitySpec* AbilitySpec)
{
    if (!AbilitySpec)
        return FGameplayTag();
        
    for (FGameplayTag Tag : AbilitySpec->GetDynamicSpecSourceTags())
    {
        if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities.Status"))))
        {
            return Tag;
        }
    }
    return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetStatusFromAbilityTag(const FGameplayTag& AbilityTag)
{
    if (const FGameplayAbilitySpec* Spec = GetSpecFromAbilityTag(AbilityTag))
    {
        return GetStatusFromSpec(*Spec);
    }
    return FGameplayTag();
}

FGameplayTag UAuraAbilitySystemComponent::GetSlotFromAbilityTag(const FGameplayTag& AbilityTag)
{
    if (const FGameplayAbilitySpec* Spec = GetSpecFromAbilityTag(AbilityTag))
    {
        return GetInputTagFromSpec(*Spec);
    }
    return FGameplayTag();
}

bool UAuraAbilitySystemComponent::SlotIsEmpty(const FGameplayTag& Slot)
{
    // 활성화된 어빌리티 잠금
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
    {
        if (AbilityHasSlot(AbilitySpec, Slot))
        {
            return false;
        }
    }
    return true;
}

FGameplayAbilitySpec* UAuraAbilitySystemComponent::GetSpecFromAbilityTag(const FGameplayTag& AbilityTag)
{
    // 활성화된 어빌리티 잠금
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
    {
        if (AbilitySpec.Ability.Get()->GetAssetTags().HasTagExact(AbilityTag))
        {
            return &AbilitySpec;
        }
    }
    return nullptr;
}

FGameplayTag UAuraAbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
    if (AbilitySpec.Ability)
    {
        for (auto& Tag : AbilitySpec.Ability.Get()->GetAssetTags())
        {
            if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(FName("Abilities"))))
            {
                return Tag;
            }
        }
    }
    return FGameplayTag();
}

void UAuraAbilitySystemComponent::UpgradeAttribute(const FGameplayTag& AttributeTag)
{
    if (GetAvatarActor()->Implements<UPlayerInterface>())
    {
        if (IPlayerInterface::Execute_GetAttributePoints(GetAvatarActor()) > 0)
        {
            ServerUpgradeAttribute(AttributeTag);
        }
    }
}

void UAuraAbilitySystemComponent::ServerUpgradeAttribute_Implementation(const FGameplayTag& AttributeTag)
{
    // 액터에게 게임플레이 이벤트 전송 -> 이벤트 수신 어빌리티 작동
    FGameplayEventData Payload;
    Payload.EventTag = AttributeTag;
    Payload.EventMagnitude = 1.f;

    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetAvatarActor(), AttributeTag, Payload);

    // 속성 포인트 감소
    if (GetAvatarActor()->Implements<UPlayerInterface>())
    {        
        IPlayerInterface::Execute_AddToAttributePoints(GetAvatarActor(), -1);
    }
}

void UAuraAbilitySystemComponent::UpdateAbilityStatus(int32 Level)
{
    static const FAuraGameplayTags& AuraTags = FAuraGameplayTags::Get();
    
    UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
    for (FAuraAbilityInfo& Info : AbilityInfo->AbilityInformation)
    {
        FGameplayTag AbilityTag = Info.AbilityTag;
        FGameplayTag InferiorAbilityTag = Info.RequireInferiorAbilityTag;
        FGameplayTag& StatusTag = Info.StatusTag;
        
        if (!AbilityTag.IsValid())
            continue;

        // 레벨 요구량보다 레벨이 낮으면 다시 잠그고 넘김
        if (Level < Info.LevelRequirement)
        {
            if (FGameplayAbilitySpec* FoundSpec = GetSpecFromAbilityTag(AbilityTag))
            {
                // 잠금
                FoundSpec->GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Locked);
                StatusTag = AuraTags.Abilities_Status_Locked;
                    
                // 즉시 복제
                MarkAbilitySpecDirty(*FoundSpec);
                ClientUpdateAbilityStatus(AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Locked, 0);
            }
            continue;
        }
        
        // 하위 스펠 보유 체크
        bool bHasInferiorSpell = false;
        if (InferiorAbilityTag.IsValid() && !InferiorAbilityTag.MatchesTag(AuraTags.Abilities_None))
        {
            auto InferiorAbilityStatus = GetStatusFromAbilityTag(InferiorAbilityTag);
            
            bHasInferiorSpell = InferiorAbilityStatus.MatchesTag(AuraTags.Abilities_Status_Equipped) ||
                InferiorAbilityStatus.MatchesTag(AuraTags.Abilities_Status_Unlocked);
        }
        else
        {
            // 하위 태그가 없을 때 
            bHasInferiorSpell = true;
        }
        
        if (!bHasInferiorSpell)
            continue;
        
        FGameplayAbilitySpec* FoundSpec = GetSpecFromAbilityTag(AbilityTag);
        // 어빌리티가 아직 부여되지 않음
        if (FoundSpec == nullptr)
        {
            // 하위 어빌리티가 필요 없거나 습득한 상태라면 진행
            FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Info.Ability, 1);
                    
            // 해금 가능 태그 부여
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Eligible);
            StatusTag = FAuraGameplayTags::Get().Abilities_Status_Eligible;
            GiveAbility(AbilitySpec);
                    
            // 즉시 복제
            MarkAbilitySpecDirty(AbilitySpec);
            ClientUpdateAbilityStatus(AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Eligible, 1);
        }
        else if (GetStatusFromSpec(FoundSpec).MatchesTag(AuraTags.Abilities_Status_Locked))
        {
            // 어빌리티가 이미 부여되었다면, 해금 가능 태그 부여
            FoundSpec->GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Eligible);
            StatusTag = FAuraGameplayTags::Get().Abilities_Status_Eligible;
                    
            // 즉시 복제
            MarkAbilitySpecDirty(*FoundSpec);
            ClientUpdateAbilityStatus(AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Eligible, 1);
        }
    }
}

void UAuraAbilitySystemComponent::ServerSpendSpellPoint_Implementation(const FGameplayTag& AbilityTag, bool bNotSpendPoint)
{
    if (GetAvatarActor()->Implements<UPlayerInterface>())
    {
        int32 SpellPoints = IPlayerInterface::Execute_GetSpellPoints(GetAvatarActor());
        if (SpellPoints <= 0)
        {
            UAuraAbilitySystemLibrary::AddMessageToActor(FGameplayTag::RequestGameplayTag("Message.NotEnoughSpellPoints"), GetAvatarActor());
            IPlayerInterface::Execute_SetSpellPoints(GetAvatarActor(), 0);
            return;
        }
        
        // 스펠 포인트 소모
        if (!bNotSpendPoint)
            IPlayerInterface::Execute_AddToSpellPoints(GetAvatarActor(), -1);
        
        // 활성화된 어빌리티
        if (FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
        {
            FGameplayTag Status = GetStatusFromSpec(*AbilitySpec);
            const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
        
            // status = unlocked, eligible, equipped
            if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Eligible))
            {
                // 상태 태그 전환
                AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GameplayTags.Abilities_Status_Eligible);
                AbilitySpec->GetDynamicSpecSourceTags().AddTag(GameplayTags.Abilities_Status_Unlocked);
                Status = GameplayTags.Abilities_Status_Unlocked;
            }
            else if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked) || Status.MatchesTagExact(GameplayTags.Abilities_Status_Equipped))
            {
                // 어빌리티 레벨 상승
                AbilitySpec->Level += 1;
            }
            else
            {
                // Locked 상태에서 포인트가 소모되었음
                return;
            }
        
            if (GetAvatarActor()->Implements<UCombatInterface>())
            {
                int32 CharacterLevel = ICombatInterface::Execute_GetCharacterLevel(GetAvatarActor()); 
                UpdateAbilityStatus(CharacterLevel);
            }
            
            // 클라이언트로 브로드캐스트
            ClientUpdateAbilityStatus(AbilityTag, Status, AbilitySpec->Level);
            MarkAbilitySpecDirty(*AbilitySpec);
        }
    }
}

void UAuraAbilitySystemComponent::Server_RefundSpellPoint_Implementation(const FGameplayTag& AbilityTag,
    bool bNotRefundPoint)
{
    // 활성화된 어빌리티
    if (FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
    {
        // 스펠 포인트 소모
        if (GetAvatarActor()->Implements<UPlayerInterface>())
        {
            if (!bNotRefundPoint)
                IPlayerInterface::Execute_AddToSpellPoints(GetAvatarActor(), 1);
        }
        
        FGameplayTag Status = GetStatusFromSpec(*AbilitySpec);
        const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

        // status = locked, unlocked, 습득가능-eligible, 장착중-equipped
        // 스펠 포인트 반환으로 인해 상태의 변화가 생김
        if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked) || Status.MatchesTagExact(GameplayTags.Abilities_Status_Equipped))
        {
            // 어빌리티 레벨 하락
            AbilitySpec->Level -= 1;
            
            // 상태 태그 전부 제거 후 조건별로 추가하기
            AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GameplayTags.Abilities_Status_Equipped);
            AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GameplayTags.Abilities_Status_Locked);
            AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GameplayTags.Abilities_Status_Unlocked);
            AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GameplayTags.Abilities_Status_Eligible);
            
            // 1. 레벨 하락으로 인해 미습득이 된 경우
            if (AbilitySpec->Level <= 0)
            {
                AbilitySpec->Level = 0;
                
                // 장착된 슬롯 해제
                const FGameplayTag Slot = GetSlotFromAbilityTag(AbilityTag);
                ClearAbilitiesOfSlot(Slot);
            }
            
            // 2. 레벨 하락만으로 바로 반영이 되었는지 확인 필요!!
            
        }

        // 어빌리티 상태 업데이트
        if (GetAvatarActor()->Implements<UCombatInterface>())
        {
            int32 CharacterLevel = ICombatInterface::Execute_GetCharacterLevel(GetAvatarActor()); 
            UpdateAbilityStatus(CharacterLevel);
        }
        
        // 클라이언트로 브로드캐스트
        ClientUpdateAbilityStatus(AbilityTag, Status, AbilitySpec->Level);
        MarkAbilitySpecDirty(*AbilitySpec);
    }
}

void UAuraAbilitySystemComponent::ServerEquipAbility_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& Slot)
{
    //
    if (FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
    {
        const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
        const FGameplayTag& Status = GetStatusFromSpec(*AbilitySpec);
        const FGameplayTag& PrevSlot = GetInputTagFromSpec(*AbilitySpec);

        // 어빌리티가 장착되었거나 해금된 경우
        const bool bStatusValid = Status == GameplayTags.Abilities_Status_Equipped || Status == GameplayTags.Abilities_Status_Unlocked;
        if (bStatusValid)
        {
            // 어빌리티 비활성화
            if (SlotIsEmpty(Slot) == false)
            {
                // 어빌리티가 슬롯에 이미 있음. 비활성화하고 슬롯 정리
                FGameplayAbilitySpec* SpecWithSlot = GetSpecWithSlot(Slot);
                if (SpecWithSlot)
                {
                    // 글로브의 태그가 교체하려는 어빌리티와 동일하면 얼리 리턴
                    if (AbilityTag.MatchesTagExact(GetAbilityTagFromSpec(*SpecWithSlot)))
                    {
                        // 메뉴 반짝임
                        ClientEquipAbility(AbilityTag, GameplayTags.Abilities_Status_Equipped, Slot, PrevSlot);
                        return;
                    }

                    // 패시브 어빌리티인가
                    if (IsPassiveAbility(*SpecWithSlot))
                    {
                        // 패시브 이펙트 비활성화
                        MulticastActivatePassiveEffect(GetAbilityTagFromSpec(*SpecWithSlot), false);
                        // 비활성화
                        DeactivePassiveAbility.Broadcast(GetAbilityTagFromSpec(*SpecWithSlot));
                    }
                    // 슬롯 정리
                    ClearSlot(SpecWithSlot);
                }
            }
            // 아직 장착 및 활성화 되지 않은 어빌리티
            if (AbilityHasAnySlot(*AbilitySpec) == false)
            {
                if (IsPassiveAbility(*AbilitySpec))
                {
                    // 어빌리티 활성화
                    TryActivateAbility(AbilitySpec->Handle);
                    // 패시브 이펙트 활성화
                    MulticastActivatePassiveEffect(AbilityTag, true);
                }
                AbilitySpec->GetDynamicSpecSourceTags().RemoveTag(GetStatusFromSpec(*AbilitySpec));
                AbilitySpec->GetDynamicSpecSourceTags().AddTag(GameplayTags.Abilities_Status_Equipped);
            }

            // 어빌리티에 입력 태그 부여
            AssignSlotToAbility(*AbilitySpec, Slot);

            // 복제
            MarkAbilitySpecDirty(*AbilitySpec);
        }
        // 메뉴 반짝임
        ClientEquipAbility(AbilityTag, GameplayTags.Abilities_Status_Equipped, Slot, PrevSlot);
    }
}


void UAuraAbilitySystemComponent::ClientEquipAbility_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PrevSlot)
{
    AbilityEquipped.Broadcast(AbilityTag, Status, Slot, PrevSlot);
}

bool UAuraAbilitySystemComponent::GetDescriptionsByAbilityTag(const FGameplayTag& AbilityTag, FString& OutDescription, FString& OutNextLevelDescription)
{
    // 어빌리티 상태 - 해금 가능(Eligible), 해금(Unlocked), 장착(Equipped)
    if (const FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
    {
        if (UAuraGameplayAbility* AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec->Ability))
        {
            auto StatusTag = GetStatusFromSpec(*AbilitySpec);
            if (!StatusTag.MatchesTag(FAuraGameplayTags::Get().Abilities_Status_Locked))
            {
                OutDescription = AuraAbility->GetDescription(AbilitySpec->Level, this);
                OutNextLevelDescription = AuraAbility->GetNextLevelDescription(AbilitySpec->Level + 1, this);
                return true;
            }
        }
    }

    // 어빌리티 상태 - 잠김(Locked)
    UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
    if (AbilityTag.IsValid() == false || AbilityTag.MatchesTagExact(FAuraGameplayTags::Get().Abilities_None))
    {
        OutDescription = FString();
    }
    else
    {
        // 이 어빌리티
        FAuraAbilityInfo* Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
        int32 CharacterLevelRequirement = Info->LevelRequirement;

        OutDescription = UAuraGameplayAbility::GetLockedDescription(CharacterLevelRequirement, this, Info->RequireInferiorAbilityTag);
    }
    OutNextLevelDescription = FString();
    return false;
}

int32 UAuraAbilitySystemComponent::GetAbilityLevelByTag(const FGameplayTag& AbilityTag)
{
    if (FGameplayAbilitySpec* Spec = GetSpecFromAbilityTag(AbilityTag))
    {
        return Spec->Level;
    }
    
    return 0;
}

void UAuraAbilitySystemComponent::ClearSlot(FGameplayAbilitySpec* Spec)
{
    // 이전 스펠에 할당된 태그 제거
    const FGameplayTag Slot = GetInputTagFromSpec(*Spec);
    const FGameplayTag StatusTag = GetStatusFromSpec(Spec);
    Spec->GetDynamicSpecSourceTags().RemoveTag(Slot);
    Spec->GetDynamicSpecSourceTags().RemoveTag(StatusTag);

    // 이전 스펠에 태그 재할당
    Spec->GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
}

void UAuraAbilitySystemComponent::ClearAbilitiesOfSlot(const FGameplayTag& Slot)
{
    FScopedAbilityListLock ActiveScopLock(*this);
    for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        if (AbilityHasSlot(Spec, Slot))
        {
            ClearSlot(&Spec);
        }
    }
}

bool UAuraAbilitySystemComponent::AbilityHasSlot(FGameplayAbilitySpec& Spec, const FGameplayTag& Slot)
{
    return Spec.GetDynamicSpecSourceTags().HasTagExact(Slot);
}

bool UAuraAbilitySystemComponent::AbilityHasAnySlot(FGameplayAbilitySpec& Spec)
{
    return Spec.GetDynamicSpecSourceTags().HasTag(FGameplayTag::RequestGameplayTag(FName("InputTag")));
}

FGameplayAbilitySpec* UAuraAbilitySystemComponent::GetSpecWithSlot(const FGameplayTag& Slot)
{
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (auto& AbilitySpec : GetActivatableAbilities())
    {
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTagExact(Slot))
        {
            return &AbilitySpec;
        }
    }
    return nullptr;
}

bool UAuraAbilitySystemComponent::IsPassiveAbility(const FGameplayAbilitySpec& Spec) const
{
    UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
    const FGameplayTag AbilityTag = GetAbilityTagFromSpec(Spec);
    if (AbilityInfo)
    {
        FAuraAbilityInfo* Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
        return Info->AbilityType.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Type_Passive);
    }
    return false;
}

void UAuraAbilitySystemComponent::AssignSlotToAbility(FGameplayAbilitySpec& Spec, const FGameplayTag& Slot)
{
    ClearSlot(&Spec);

    // 입력 태그 부여
    Spec.GetDynamicSpecSourceTags().AddTag(Slot);
}

void UAuraAbilitySystemComponent::MulticastActivatePassiveEffect_Implementation(const FGameplayTag& AbilityTag, bool bActivate)
{
    ActivatePassiveEffect.Broadcast(AbilityTag, bActivate);
}

void UAuraAbilitySystemComponent::MessageRemove(const FGameplayTag& Tag)
{
    OnMessageRemoved.Broadcast(Tag);
}

void UAuraAbilitySystemComponent::OnRep_ActivateAbilities()
{
    Super::OnRep_ActivateAbilities();

    // 어빌리티가 처음 부여될 때만 발동
    if (bStartupAbilitiesGiven == false)
    {
        bStartupAbilitiesGiven = true;
        AbilitiesGivenDelegate.Broadcast();
    }
}

void UAuraAbilitySystemComponent::ClientUpdateAbilityStatus_Implementation(const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 AbilityLevel)
{
    // 
    AbilityStatusChanged.Broadcast(AbilityTag, StatusTag, AbilityLevel);
}

void UAuraAbilitySystemComponent::ClientEffectApplied_Implementation(UAbilitySystemComponent *AbilitySystemComponent, const FGameplayEffectSpec &EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
    // 이펙트 적용 시 무엇을 BroadCast 할 것인가??
    // Tag가 적절
    FGameplayTagContainer TagContainer;
    EffectSpec.GetAllAssetTags(TagContainer);

    EffectAssetTags.Broadcast(TagContainer);
}