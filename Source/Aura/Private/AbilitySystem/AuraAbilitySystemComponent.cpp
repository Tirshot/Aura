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
                GiveAbilityAndActivateOnce(LoadedAbilitySpec);
            }
            else
            {
                GiveAbility(LoadedAbilitySpec);
            }
        }
    }
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
    UpdateAbilityStatus(SaveData->PlayerLevel);
}

void UAuraAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
    for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
    {
        // 게임플레이 어빌리티 스펙 생성
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);

        // 어빌리티에 입력 태그를 부여
        if (const UAuraGameplayAbility* AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability))
        {
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraAbility->StartupInputTag);
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
            
            GiveAbility(AbilitySpec);
        }
    }
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
    UpdateAbilityStatus(1);
}

void UAuraAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
    for (TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
    {
        // 게임플레이 어빌리티 스펙 생성
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
        AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
        GiveAbilityAndActivateOnce(AbilitySpec);
    }

    if (GetAvatarActor()->Implements<UCombatInterface>())
    {
        int32 Level = 0;
        Level = ICombatInterface::Execute_GetCharacterLevel(GetAvatarActor());
        UpdateAbilityStatus(Level);
    }
}

void UAuraAbilitySystemComponent::AddCharacterAbility(const FGameplayTag& AbilityTag)
{
    // 게임플레이 어빌리티 스펙 생성
    auto AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this)->FindAbilityInfoForTag(AbilityTag);
    if (auto Ability = AbilityInfo.Ability)
    {
        int AbilityLevel = GetAbilityLevelByTag(AbilityTag);
        
        // 이미 가지고 있는 어빌리티
        if (AbilityLevel > 0)
        {
            FScopedAbilityListLock ActiveScopeLoc(*this);
            for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
            {
                // 활성화 된 모든 어빌리티 스펙에서 태그와 일치하는 스펙 찾기
                if (AbilitySpec.Ability->GetAssetTags().HasTagExact(AbilityTag))
                {
                    // 어빌리티 레벨업
                    AbilitySpec.Level += 1;
                    break;
                }
            }
        }
        else
        {
            FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1);
        
            // 어빌리티에 입력 태그를 부여
            if (const UAuraGameplayAbility* AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability))
            {
                AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraAbility->StartupInputTag);
                AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
            
                GiveAbility(AbilitySpec);
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
            }
            CancelAbilityHandle(AbilitySpec.Handle);
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
        for (FGameplayTag Tag : AbilitySpec.Ability.Get()->GetAssetTags())
        {
            if (Tag.MatchesTag(AbilityTag))
            {
                return &AbilitySpec;
            }
        }
    }
    return nullptr;
}

FGameplayTag UAuraAbilitySystemComponent::GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec)
{
    if (AbilitySpec.Ability)
    {
        for (FGameplayTag Tag : AbilitySpec.Ability.Get()->GetAssetTags())
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
    UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(GetAvatarActor());
    for (const FAuraAbilityInfo& Info : AbilityInfo->AbilityInformation)
    {
        if (Info.AbilityTag.IsValid() == false)
            continue;

        // 레벨 요구량보다 레벨이 낮으면 넘김
        if (Level < Info.LevelRequirement)
            continue;

        // 활성화된 어빌리티 중 스펙을 찾지 못하였다면 생성 후 어빌리티 부여
        if (GetSpecFromAbilityTag(Info.AbilityTag) == nullptr)
        {
            auto* Spec = GetSpecFromAbilityTag(Info.RequireInferiorAbilityTag);
            auto InferiorAbilityStatus= GetStatusFromSpec(Spec);
            
            // 하위 스펠 습득 확인
            if (InferiorAbilityStatus == FAuraGameplayTags::Get().Abilities_Status_Equipped
                || InferiorAbilityStatus == FAuraGameplayTags::Get().Abilities_Status_Unlocked)
            {
                FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Info.Ability, 1);

                // 해금 가능 태그 부여
                AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Eligible);
                GiveAbility(AbilitySpec);

                // 즉시 복제
                MarkAbilitySpecDirty(AbilitySpec);
                ClientUpdateAbilityStatus(Info.AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Eligible, 1);
            }
            else if (Info.RequireInferiorAbilityTag.MatchesTag(FGameplayTag::RequestGameplayTag("Abilities.None"))
                || Info.RequireInferiorAbilityTag.MatchesTag(FGameplayTag()))
            {
                FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Info.Ability, 1);

                // 해금 가능 태그 부여
                AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Eligible);
                GiveAbility(AbilitySpec);

                // 즉시 복제
                MarkAbilitySpecDirty(AbilitySpec);
                ClientUpdateAbilityStatus(Info.AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Eligible, 1);
            }
        }
    }
}

void UAuraAbilitySystemComponent::ServerSpendSpellPoint_Implementation(const FGameplayTag& AbilityTag, bool bNotSpendPoint)
{
    // 활성화된 어빌리티
    if (FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
    {
        // 스펠 포인트 소모
        if (GetAvatarActor()->Implements<UPlayerInterface>())
        {
            if (!bNotSpendPoint)
                IPlayerInterface::Execute_AddToSpellPoints(GetAvatarActor(), -1);
        }
        
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
            OutDescription = AuraAbility->GetDescription(AbilitySpec->Level, this);
            OutNextLevelDescription = AuraAbility->GetNextLevelDescription(AbilitySpec->Level + 1, this);
            return true;
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
        FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
        int32 CharacterLevelRequirement = Info.LevelRequirement;

        OutDescription = UAuraGameplayAbility::GetLockedDescription(CharacterLevelRequirement, this, Info.RequireInferiorAbilityTag);
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
    //
    const FGameplayTag Slot = GetInputTagFromSpec(*Spec);
    Spec->GetDynamicSpecSourceTags().RemoveTag(Slot);
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
        FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
        return Info.AbilityType.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Type_Passive);
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