// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "Interaction/PlayerInterface.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Character/AuraCharacter.h"
#include "Game/LoadScreenSaveGame.h"
#include "Interaction/CombatInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"

void UAuraAbilitySystemComponent::AbilityActorInfoSet()
{
    OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UAuraAbilitySystemComponent::ClientEffectApplied);
}

void UAuraAbilitySystemComponent::BeginDestroy()
{
    AbilityStatusChanged.Clear();
    AbilityEquipped.Clear();
    EffectAssetTags.Clear();
    AbilitiesGivenDelegate.Clear();
    DeactivePassiveAbility.Clear();
    ActivatePassiveEffect.Clear();
    OnMessageTagReceived.Clear();
    OnGameplayEffectAppliedDelegateToSelf.Clear();

    Super::BeginDestroy();
}

void UAuraAbilitySystemComponent::AddCharacterAbilitiesFromSaveData(ULoadScreenSaveGame* SaveData)
{
    // 저장된 어빌리티 순회
    for (const FSavedAbility& Data : SaveData->SavedAbilities)
    {
        const TSubclassOf<UGameplayAbility> LoadedAbilityClass = Data.GameplayAbility;
        
        FGameplayAbilitySpec* ExistingSpec = GetSpecFromAbilityTag(Data.AbilityTag);
        if (ExistingSpec)
        {
            // 이미 보유중이면 넘기기
            ExistingSpec->Level = Data.AbilityLevel;
            continue;
        }
        
        FGameplayAbilitySpec LoadedAbilitySpec = FGameplayAbilitySpec(LoadedAbilityClass, Data.AbilityLevel);
        LoadedAbilitySpec.GetDynamicSpecSourceTags().AddTag(Data.AbilitySlot);
        LoadedAbilitySpec.GetDynamicSpecSourceTags().AddTag(Data.AbilityStatus);
        
        if (Data.AbilityType == FAuraGameplayTags::Get().Abilities_Type_Offensive)
        {
            GiveAbility(LoadedAbilitySpec);
            
            // 시작 태그가 동일할 경우 덮어씌우기
            FScopedAbilityListLock ActiveScopeLoc(*this);
            for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
            {
                if (Spec.Ability == LoadedAbilitySpec.Ability)
                    continue;
        
                if (Spec.GetDynamicSpecSourceTags().HasTag(Data.AbilitySlot))
                {
                    // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                    Spec.GetDynamicSpecSourceTags().RemoveTag(Data.AbilitySlot);
                    Spec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                    Spec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                }
            }
        }
        else if (Data.AbilityType == FAuraGameplayTags::Get().Abilities_Type_Passive)
        {
            if (Data.AbilityTag.MatchesTag(FGameplayTag::RequestGameplayTag("Abilities.Passive.ListenForEvent")))
            {
                GiveAbility(LoadedAbilitySpec);
                TryActivateAbility(LoadedAbilitySpec.Handle);
            }
            if (Data.AbilityStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
            {
                GiveAbilityAndActivateOnce(LoadedAbilitySpec);
            }
            
            // 시작 태그가 동일할 경우 덮어씌우기
            FScopedAbilityListLock ActiveScopeLoc(*this);
            for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
            {
                if (Spec.Ability == LoadedAbilitySpec.Ability || LoadedAbilitySpec.Ability == nullptr)
                    continue;
        
                if (Spec.GetDynamicSpecSourceTags().HasTag(Data.AbilitySlot))
                {
                    // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                    // 패시브 비활성화
                    Spec.GetDynamicSpecSourceTags().RemoveTag(Data.AbilitySlot);
                    Spec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                    Spec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                    DeactivePassiveAbility.Broadcast(GetAbilityTagFromSpec(Spec));
                }
            }
        }
    }
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
    UpdateAbilityStatus(SaveData->PlayerLevel);
}

void UAuraAbilitySystemComponent::AddCharacterAbilitiesFromArray(const TArray<FSavedAbility>& SavedAbilities)
{
    // 저장된 어빌리티 순회
    for (const FSavedAbility& Data : SavedAbilities)
    {
        const TSubclassOf<UGameplayAbility> LoadedAbilityClass = Data.GameplayAbility;
        
        FGameplayAbilitySpec* ExistingSpec = GetSpecFromAbilityTag(Data.AbilityTag);
        if (ExistingSpec)
        {
            // 이미 보유중이면 넘기기
            ExistingSpec->Level = Data.AbilityLevel;
            continue;
        }
        
        FGameplayAbilitySpec LoadedAbilitySpec = FGameplayAbilitySpec(LoadedAbilityClass, Data.AbilityLevel);
        LoadedAbilitySpec.GetDynamicSpecSourceTags().AddTag(Data.AbilitySlot);
        LoadedAbilitySpec.GetDynamicSpecSourceTags().AddTag(Data.AbilityStatus);
        
        if (Data.AbilityType == FAuraGameplayTags::Get().Abilities_Type_Offensive)
        {
            GiveAbility(LoadedAbilitySpec);
            
            // 시작 태그가 동일할 경우 덮어씌우기
            FScopedAbilityListLock ActiveScopeLoc(*this);
            for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
            {
                if (Spec.Ability == LoadedAbilitySpec.Ability)
                    continue;
        
                if (Spec.GetDynamicSpecSourceTags().HasTag(Data.AbilitySlot))
                {
                    // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                    Spec.GetDynamicSpecSourceTags().RemoveTag(Data.AbilitySlot);
                    Spec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                    Spec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                }
            }
        }
        else if (Data.AbilityType == FAuraGameplayTags::Get().Abilities_Type_Passive)
        {
            if (Data.AbilityTag.MatchesTag(FGameplayTag::RequestGameplayTag("Abilities.Passive.ListenForEvent")))
            {
                GiveAbility(LoadedAbilitySpec);
                TryActivateAbility(LoadedAbilitySpec.Handle);
            }
            if (Data.AbilityStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
            {
                GiveAbilityAndActivateOnce(LoadedAbilitySpec);
            }
            
            // 시작 태그가 동일할 경우 덮어씌우기
            FScopedAbilityListLock ActiveScopeLoc(*this);
            for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
            {
                if (Spec.Ability == LoadedAbilitySpec.Ability || LoadedAbilitySpec.Ability == nullptr)
                    continue;
        
                if (Spec.GetDynamicSpecSourceTags().HasTag(Data.AbilitySlot))
                {
                    // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                    // 패시브 비활성화
                    Spec.GetDynamicSpecSourceTags().RemoveTag(Data.AbilitySlot);
                    Spec.GetDynamicSpecSourceTags().RemoveTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
                    Spec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Unlocked);
                    DeactivePassiveAbility.Broadcast(GetAbilityTagFromSpec(Spec));
                }
            }
        }
    }
    
    int32 PlayerLevel = ICombatInterface::Execute_GetCharacterLevel(GetAvatarActor());
    
    bStartupAbilitiesGiven = true;
    AbilitiesGivenDelegate.Broadcast();
    UpdateAbilityStatus(PlayerLevel);
}

void UAuraAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities)
{
    UAuraGameplayAbility* AuraAbility = nullptr;
    FGameplayTag StartupInputTag;
    
    for (TSubclassOf<UGameplayAbility> AbilityClass : StartupAbilities)
    {
        // 게임플레이 어빌리티 스펙 생성
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
        FGameplayAbilitySpec* ExistingSpec = GetSpecFromAbilityTag(GetAbilityTagFromSpec(AbilitySpec));

        AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability);
        
        // 어빌리티에 입력 태그를 부여
        if (AuraAbility)
        {
            StartupInputTag = AuraAbility->StartupInputTag;
            AuraAbility->bIsStartupAbility = true;
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
        FGameplayAbilitySpec* ExistingSpec = GetSpecFromAbilityTag(GetAbilityTagFromSpec(AbilitySpec));

        AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability);
        
        if (AuraAbility)
        {
            StartupInputTag = AuraAbility->StartupInputTag;
            AuraAbility->bIsStartupAbility = true;
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupInputTag);
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(FAuraGameplayTags::Get().Abilities_Status_Equipped);
        }
        
        if (ExistingSpec)
        {
            // 이미 보유중이면 넘기기
            ExistingSpec->Level = AbilitySpec.Ability->GetAbilityLevel();
            TryActivateAbility(ExistingSpec->Handle);
        }
        else
        {
            // GA_ListenForEvent는 오라 어빌리티가 아님!!
            if (GetAbilityTagFromSpec(AbilitySpec).MatchesAny(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Abilities.Passive.ListenForEvent"))))
            {
                GiveAbility(AbilitySpec);
                TryActivateAbility(AbilitySpec.Handle);
                continue;
            }
            GiveAbilityAndActivateOnce(AbilitySpec);
        }
        
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
    static const FAuraGameplayTags& AuraTags = FAuraGameplayTags::Get();
    
    // 아직 부여되지 않은 어빌리티
    UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this);
    if (!AbilityInfo)
        return;
    
    FAuraAbilityInfo* AuraAbilityInfo = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
    if (!AuraAbilityInfo)
        return;
    
    // 이미 부여된 어빌리티인지 체크
    if (auto* FoundSpec = GetSpecFromAbilityTag(AbilityTag))
    {
        FGameplayTag StatusTag = GetStatusFromSpec(FoundSpec);
        
        // 해금 가능 상태였다면 습득 상태로 변경
        if (StatusTag.MatchesTag(AuraTags.Abilities_Status_Eligible))
        {
            FoundSpec->GetDynamicSpecSourceTags().RemoveTag(AuraTags.Abilities_Status_Eligible);
            FoundSpec->GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Unlocked);
            AuraAbilityInfo->StatusTag = AuraTags.Abilities_Status_Unlocked;
        }
        
        // 습득, 장착 상태일 때는 레벨업
        if (StatusTag.MatchesTag(AuraTags.Abilities_Status_Unlocked) ||
            StatusTag.MatchesTag(AuraTags.Abilities_Status_Equipped))
        {
            FoundSpec->Level += 1;
        }
        MarkAbilitySpecDirty(*FoundSpec);
        
        // 자동 장착
        auto AuraAbility = Cast<UAuraGameplayAbility>(FoundSpec->Ability);
        if (!AuraAbility)
            return;
                
        FGameplayTag StartupInputTag = AuraAbility->StartupInputTag;
        FoundSpec->GetDynamicSpecSourceTags().AddTag(StartupInputTag);
        FoundSpec->GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Equipped);
        AuraAbilityInfo->StatusTag = AuraTags.Abilities_Status_Equipped;
        AuraAbilityInfo->InputTag = StartupInputTag;
                
        if (StartupInputTag.IsValid() && !StartupInputTag.MatchesTag(AuraTags.Abilities_None))
        {
            FoundSpec->GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Unlocked);
        }
            
        // 시작 태그가 동일할 경우 덮어씌우기
        FScopedAbilityListLock ActiveScopeLoc(*this);
        for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
        {
            AuraAbilityInfo = AbilityInfo->FindAbilityInfoForTag(GetAbilityTagFromSpec(Spec));
                    
            if (Spec.Handle == FoundSpec->Handle)
                continue;
        
            if (Spec.GetDynamicSpecSourceTags().HasTag(StartupInputTag))
            {
                // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                Spec.GetDynamicSpecSourceTags().RemoveTag(StartupInputTag);
                Spec.GetDynamicSpecSourceTags().RemoveTag(AuraTags.Abilities_Status_Equipped);
                Spec.GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Unlocked);
                AuraAbilityInfo->StatusTag = AuraTags.Abilities_Status_Unlocked;
                AuraAbilityInfo->InputTag = FGameplayTag::EmptyTag;
            }
        }
        ClientEquipAbility(AbilityTag, AuraTags.Abilities_Status_Equipped, StartupInputTag, FGameplayTag::EmptyTag);
    }
    else
    {
        // 아직 부여되지 않은 어빌리티
        AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this);
        AuraAbilityInfo = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
                
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AuraAbilityInfo->Ability, 1);
                
        // 어빌리티에 입력 태그를 부여
        auto AuraAbility = Cast<UAuraGameplayAbility>(AbilitySpec.Ability);
        if (!AuraAbility)
            return;
                
        FGameplayTag StartupInputTag = AuraAbility->StartupInputTag;
        AbilitySpec.GetDynamicSpecSourceTags().AddTag(StartupInputTag);
        AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Equipped);
        AuraAbilityInfo->StatusTag = AuraTags.Abilities_Status_Equipped;
        AuraAbilityInfo->InputTag = StartupInputTag;
                
        if (StartupInputTag.IsValid() && !StartupInputTag.MatchesTag(AuraTags.Abilities_None))
        {
            // 입력 태그가 없는 경우(패시브 등)는 Unlocked 상태로 시작
            AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Unlocked);
        }
                
        FGameplayAbilitySpecHandle AddedAbilityHandle;
        if (AbilitySpec.GetDynamicSpecSourceTags().HasTag(AuraTags.Abilities_Passive))
        { // 패시브면
            AddedAbilityHandle = GiveAbilityAndActivateOnce(AbilitySpec);
            MulticastActivatePassiveEffect(AbilityTag, true);
        }
        else
        { // 액티브면
            AddedAbilityHandle = GiveAbility(AbilitySpec);
        }
            
        // 시작 태그가 동일할 경우 덮어씌우기
        FScopedAbilityListLock ActiveScopeLoc(*this);
        for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
        {
            AuraAbilityInfo = AbilityInfo->FindAbilityInfoForTag(GetAbilityTagFromSpec(Spec));
                
            if (Spec.Handle == AddedAbilityHandle)
                continue;
        
            if (Spec.GetDynamicSpecSourceTags().HasTag(StartupInputTag))
            {
                // 위에서 부여한 어빌리티가 아닌 어빌리티의 입력태그와 겹치면 해당 스펙에서 입력 태그 제거
                Spec.GetDynamicSpecSourceTags().RemoveTag(StartupInputTag);
                Spec.GetDynamicSpecSourceTags().RemoveTag(AuraTags.Abilities_Status_Equipped);
                Spec.GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Unlocked);
                AuraAbilityInfo->StatusTag = AuraTags.Abilities_Status_Unlocked;
                AuraAbilityInfo->InputTag = FGameplayTag::EmptyTag;
            }
                    
            if (AbilitySpec.GetDynamicSpecSourceTags().HasTag(AuraTags.Abilities_Passive))
            {
                MulticastActivatePassiveEffect(AbilityTag, false);
                DeactivePassiveAbility.Broadcast(AbilityTag);
            }
        }
        ClientEquipAbility(AbilityTag, AuraTags.Abilities_Status_Equipped, StartupInputTag, FGameplayTag::EmptyTag);
    }
    
    AbilitiesGivenDelegate.Broadcast();
    
    if (AbilityActorInfo.IsValid())
    {
        if (APlayerState* PS = AbilityActorInfo->PlayerController->PlayerState)
        {
            if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(PS))
            {
                UpdateAbilityStatus(AuraPS->GetCharacterLevel());
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("AuraASC::AddCharacterAbilityByTag, UpdateAbilityStatus FAILED!!!"));
    }
}

void UAuraAbilitySystemComponent::RemoveCharacterAbilityByTag(const FGameplayTag& AbilityTag, const int32 RemoveCount)
{
    static const FAuraGameplayTags& AuraTags = FAuraGameplayTags::Get();
    FGameplayAbilitySpec* FoundSpec = GetSpecFromAbilityTag(AbilityTag);
    if (!FoundSpec)
        return;

    int32 NewLevel = FoundSpec->Level - RemoveCount;
    FoundSpec->Level = FMath::Max(0, NewLevel);

    if (FoundSpec->Level == 0)
    {
        // 장착 해제 및 상태 초기화
        FoundSpec->GetDynamicSpecSourceTags().RemoveTag(AuraTags.Abilities_Status_Equipped);
        FoundSpec->GetDynamicSpecSourceTags().RemoveTag(AuraTags.Abilities_Status_Unlocked);
        FoundSpec->GetDynamicSpecSourceTags().RemoveTag(AuraTags.Abilities_Status_Eligible);
        
        // 입력 태그 제거
        FGameplayTag InputTag = GetInputTagFromSpec(*FoundSpec);
        if (InputTag.IsValid())
        {
            FoundSpec->GetDynamicSpecSourceTags().RemoveTag(InputTag);
        }

        // 상태를 잠금으로 변경
        FoundSpec->GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Locked);

        // 실행 중인 어빌리티 중단
        CancelAbilityHandle(FoundSpec->Handle);

        bool bIsPassive = AbilityTag.MatchesTag(AuraTags.Abilities_Passive);
        if (bIsPassive)
        {
            MulticastActivatePassiveEffect(AbilityTag, false);
            DeactivePassiveAbility.Broadcast(AbilityTag);
        }
        
        FoundSpec->Level = 1;
    
        FAuraAbilityInfo LastSlotInfo;
        LastSlotInfo.StatusTag = AuraTags.Abilities_Status_Unlocked;
        LastSlotInfo.InputTag = InputTag;
        LastSlotInfo.AbilityTag = AuraTags.Abilities_None;
        
        // 슬롯 비우기
        if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetOwner()))
        {
            if (APlayerController* PC = AuraPS->GetPlayerController())
                UAuraAbilitySystemLibrary::GetOverlayWidgetController(PC)->AbilityInfoDelegate.Broadcast(LastSlotInfo);
        }
    }

    MarkAbilitySpecDirty(*FoundSpec);
    
    AbilitiesGivenDelegate.Broadcast();
    

    if (AbilityActorInfo.IsValid())
    {
        if (APlayerState* PS = AbilityActorInfo->PlayerController->PlayerState)
        {
            if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(PS))
            {
                UpdateAbilityStatus(AuraPS->GetCharacterLevel());
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
        Delegate.ExecuteIfBound(AbilitySpec);
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
    if (!AbilityInfo)
        return;
    
    for (FAuraAbilityInfo& Info : AbilityInfo->AbilityInformation)
    {
        FGameplayTag AbilityTag = Info.AbilityTag;
        FGameplayAbilitySpec* FoundSpec = GetSpecFromAbilityTag(AbilityTag);
        if (FoundSpec)
        {
            // 이미 해금된 어빌리티는 잠그지 않음
            FGameplayTag StatusTag = GetStatusFromSpec(FoundSpec);
            if (StatusTag.MatchesTag(AuraTags.Abilities_Status_Unlocked) || 
                StatusTag.MatchesTag(AuraTags.Abilities_Status_Equipped))
            {
                continue; 
            }

            // 시작 어빌리티라면 잠금 로직 패스
            if (auto AuraAbility = Cast<UAuraGameplayAbility>(FoundSpec->Ability))
            {
                if (AuraAbility->bIsStartupAbility)
                    continue;
            }
            else // AuraAbility가 아니어도 패스
            {
                continue;
            }
        }
        // 레벨 요구량보다 레벨이 낮으면 다시 잠그고 넘김
        if (Level < Info.LevelRequirement)
        {
            if (FoundSpec && GetStatusFromSpec(FoundSpec).MatchesTag(AuraTags.Abilities_Status_Eligible))
            {
                // 잠금
                FoundSpec->GetDynamicSpecSourceTags().RemoveTag(AuraTags.Abilities_Status_Eligible);
                FoundSpec->GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Locked);
                if (auto AuraAbility = Cast<UAuraGameplayAbility>(FoundSpec->Ability))
                {
                    FoundSpec->GetDynamicSpecSourceTags().RemoveTag(AuraAbility->StartupInputTag);
                }
                Info.StatusTag = AuraTags.Abilities_Status_Locked;
                Info.InputTag = FGameplayTag::EmptyTag;
                    
                // 즉시 복제
                MarkAbilitySpecDirty(*FoundSpec);
                ClientUpdateAbilityStatus(AbilityTag, AuraTags.Abilities_Status_Locked, 1);
            }
            continue;
        }
        
        // 하위 스펠 보유 체크
        bool bPassInferiorSpell = false;
        bool bNeedInferiorSpell = false;
        FGameplayTag InferiorTag = Info.RequireInferiorAbilityTag;
        if (InferiorTag.IsValid() && !InferiorTag.MatchesTag(AuraTags.Abilities_None))
        {
            FGameplayTag InferiorStatus = GetStatusFromAbilityTag(InferiorTag);
            bPassInferiorSpell = InferiorStatus.MatchesTag(AuraTags.Abilities_Status_Unlocked) || 
                                 InferiorStatus.MatchesTag(AuraTags.Abilities_Status_Equipped);
            bNeedInferiorSpell = true;
        }
        
        // 어빌리티가 아직 부여되지 않음
        if (FoundSpec == nullptr)
        {
            if (bPassInferiorSpell || !bNeedInferiorSpell)
            {
                // 하위 어빌리티가 필요 없거나 습득한 상태라면 진행
                FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Info.Ability, 1);
                    
                // 해금 가능 태그 부여
                AbilitySpec.GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Eligible);
                GiveAbility(AbilitySpec);
                    
                // 즉시 복제
                MarkAbilitySpecDirty(AbilitySpec);
                ClientUpdateAbilityStatus(AbilityTag, AuraTags.Abilities_Status_Eligible, 1);
            }
        }
        else if (GetStatusFromSpec(FoundSpec).MatchesTag(AuraTags.Abilities_Status_Locked))
        {
            if (bPassInferiorSpell || !bNeedInferiorSpell)
            {
                // 어빌리티가 이미 부여되었다면, 해금 가능 태그 부여
                FoundSpec->GetDynamicSpecSourceTags().RemoveTag(AuraTags.Abilities_Status_Locked);
                FoundSpec->GetDynamicSpecSourceTags().AddTag(AuraTags.Abilities_Status_Eligible);
                Info.StatusTag = AuraTags.Abilities_Status_Eligible;
                
                // 즉시 복제
                MarkAbilitySpecDirty(*FoundSpec);
                ClientUpdateAbilityStatus(AbilityTag, AuraTags.Abilities_Status_Eligible, 1);
            }
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
            UAuraAbilitySystemLibrary::AddMessageToActor(Cast<AAuraCharacter>(GetAvatarActor()), FGameplayTag::RequestGameplayTag("Message.NotEnoughSpellPoints"));
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