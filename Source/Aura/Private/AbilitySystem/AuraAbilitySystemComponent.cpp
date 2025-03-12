// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraLogChannels.h"
#include "Interaction/PlayerInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityInfo.h"

void UAuraAbilitySystemComponent::AbilityActorInfoSet()
{
    OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAuraAbilitySystemComponent::ClientEffectApplied);
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
            AbilitySpec.DynamicAbilityTags.AddTag(AuraAbility->StartupInputTag);
            AbilitySpec.DynamicAbilityTags.AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
            GiveAbility(AbilitySpec);
        }
    }
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
}

void UAuraAbilitySystemComponent::AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities)
{
    for (TSubclassOf<UGameplayAbility> AbilityClass : StartupPassiveAbilities)
    {
        // 게임플레이 어빌리티 스펙 생성
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
        GiveAbilityAndActivateOnce(AbilitySpec);
    }
}

void UAuraAbilitySystemComponent::AbilityInputTagPressed(const FGameplayTag& InputTag)
{
    // 입력 태그가 유효하지 않으면 종료
    if (!InputTag.IsValid())
        return;

    for (auto& AbilitySpec : GetActivatableAbilities())
    {
        // 태그 컨테이너를 순회하여 입력 태그와 일치하는 어빌리티 스펙을 찾음
        if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
        {
            // 입력 확인
            AbilitySpecInputPressed(AbilitySpec);

            // 활성화 되어 있지 않으면 활성화
            if (!AbilitySpec.IsActive())
            {
                InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
            }
        }
    }
}

void UAuraAbilitySystemComponent::AbilityInputTagHeld(const FGameplayTag& InputTag)
{
    // 입력 태그가 유효하지 않으면 종료
    if (!InputTag.IsValid())
        return;

    for (auto& AbilitySpec : GetActivatableAbilities())
    {
        // 태그 컨테이너를 순회하여 입력 태그와 일치하는 어빌리티 스펙을 찾음
        if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag))
        {
            // 입력 확인
            AbilitySpecInputPressed(AbilitySpec);

            // 활성화 되어 있지 않으면 활성화
            if (!AbilitySpec.IsActive())
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

    for (auto& AbilitySpec : GetActivatableAbilities())
    {
        // 태그 컨테이너를 순회하여 입력 태그와 일치하는 어빌리티 스펙을 찾음
        if (AbilitySpec.DynamicAbilityTags.HasTagExact(InputTag) && AbilitySpec.IsActive())
        {
            // 입력 확인
            AbilitySpecInputReleased(AbilitySpec);
            InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputReleased, AbilitySpec.Handle, AbilitySpec.ActivationInfo.GetActivationPredictionKey());
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
    for (FGameplayTag Tag : AbilitySpec.DynamicAbilityTags)
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
    for (FGameplayTag Tag : AbilitySpec.DynamicAbilityTags)
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

FGameplayTag UAuraAbilitySystemComponent::GetInputTagFromAbilityTag(const FGameplayTag& AbilityTag)
{
    if (const FGameplayAbilitySpec* Spec = GetSpecFromAbilityTag(AbilityTag))
    {
        return GetInputTagFromSpec(*Spec);
    }
    return FGameplayTag();
}

FGameplayAbilitySpec* UAuraAbilitySystemComponent::GetSpecFromAbilityTag(const FGameplayTag& AbilityTag)
{
    // 활성화된 어빌리티 잠금
    FScopedAbilityListLock ActiveScopeLock(*this);
    for (FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
    {
        for (FGameplayTag Tag : AbilitySpec.Ability.Get()->AbilityTags)
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
        for (FGameplayTag Tag : AbilitySpec.Ability.Get()->AbilityTags)
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
            FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Info.Ability, 1);

            // 해금 가능 태그 부여
            AbilitySpec.DynamicAbilityTags.AddTag(FAuraGameplayTags::Get().Abilities_Status_Eligible);
                        
            GiveAbility(AbilitySpec);

            // 즉시 복제
            MarkAbilitySpecDirty(AbilitySpec);
            ClientUpdateAbilityStatus(Info.AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Eligible, 1);
        }
    }
}

void UAuraAbilitySystemComponent::ServerSpendSpellPoint_Implementation(const FGameplayTag& AbilityTag)
{
    // 활성화된 어빌리티
    if (FGameplayAbilitySpec* AbilitySpec = GetSpecFromAbilityTag(AbilityTag))
    {
        // 스펠 포인트 소모
        if (GetAvatarActor()->Implements<UPlayerInterface>())
        {
            IPlayerInterface::Execute_AddToSpellPoints(GetAvatarActor(), -1);
        }
        
        FGameplayTag Status = GetStatusFromSpec(*AbilitySpec);
        const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();

        // status = unlocked, eligible, equipped
        if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Eligible))
        {
            // 상태 태그 전환
            AbilitySpec->DynamicAbilityTags.RemoveTag(GameplayTags.Abilities_Status_Eligible);
            AbilitySpec->DynamicAbilityTags.AddTag(GameplayTags.Abilities_Status_Unlocked);
            Status = GameplayTags.Abilities_Status_Unlocked;
        }
        else if (Status.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked) || Status.MatchesTagExact(GameplayTags.Abilities_Status_Equipped))
        {
            // 어빌리티 레벨 상승
            AbilitySpec->Level += 1;
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
        // 슬롯 바꾸기
        const FGameplayTag& PrevSlot = GetInputTagFromSpec(*AbilitySpec);
        const FGameplayTag& Status = GetStatusFromSpec(*AbilitySpec);

        // 어빌리티가 장착되었거나 해금된 경우
        const bool bStatusValid = Status == FAuraGameplayTags::Get().Abilities_Status_Equipped || Status == FAuraGameplayTags::Get().Abilities_Status_Unlocked;
        if (bStatusValid)
        {
            // 해당 슬롯에 있는 어빌리티를 제거
            ClearAbilitiesOfSlot(Slot);
            // 이 어빌리티 슬롯을 초기화
            ClearSlot(AbilitySpec);
            // 슬롯 바꿔치기
            AbilitySpec->DynamicAbilityTags.AddTag(Slot);

            // 어빌리티가 해금된 경우
            if (Status.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Unlocked))
            {
                // 장착 태그 부여
                AbilitySpec->DynamicAbilityTags.RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                AbilitySpec->DynamicAbilityTags.AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
            }
            // 복제
            MarkAbilitySpecDirty(*AbilitySpec);
        }
        // 클라이언트 RPC
        ClientEquipAbility(AbilityTag, FAuraGameplayTags::Get().Abilities_Status_Equipped, Slot, PrevSlot);
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
            OutDescription = AuraAbility->GetDescription(AbilitySpec->Level);
            OutNextLevelDescription = AuraAbility->GetNextLevelDescription(AbilitySpec->Level + 1);
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
        OutDescription = UAuraGameplayAbility::GetLockedDescription(AbilityInfo->FindAbilityInfoForTag(AbilityTag).LevelRequirement);
    }
    OutNextLevelDescription = FString();
    return false;
}

void UAuraAbilitySystemComponent::ClearSlot(FGameplayAbilitySpec* Spec)
{
    //
    const FGameplayTag Slot = GetInputTagFromSpec(*Spec);
    Spec->DynamicAbilityTags.RemoveTag(Slot);
    MarkAbilitySpecDirty(*Spec);
}

void UAuraAbilitySystemComponent::ClearAbilitiesOfSlot(const FGameplayTag& Slot)
{
    FScopedAbilityListLock ActiveScopLock(*this);
    for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        if (AbilityHasSlot(&Spec, Slot))
        {
            ClearSlot(&Spec);
        }
    }
}

bool UAuraAbilitySystemComponent::AbilityHasSlot(FGameplayAbilitySpec* Spec, const FGameplayTag& Slot)
{
    for (FGameplayTag Tag : Spec->DynamicAbilityTags)
    {
        if (Tag.MatchesTagExact(Slot))
        {
            return true;
        }
    }
    return false;
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