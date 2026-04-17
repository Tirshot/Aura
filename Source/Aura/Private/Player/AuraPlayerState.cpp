// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerState.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Net/UnrealNetwork.h"
#include "Player/CharmComponent.h"
#include "Player/EquipmentComponent.h"
#include "Player/InventoryComponent.h"


void FOwnedAbilityUpgrade::PostReplicatedAdd(const FOwnedAbilityUpgradeList& InArraySerializer)
{
    
}

void FOwnedAbilityUpgrade::PostReplicatedChange(const FOwnedAbilityUpgradeList& InArraySerializer)
{
    
}

void FOwnedAbilityUpgrade::PreReplicatedRemove(const FOwnedAbilityUpgradeList& InArraySerializer)
{
    
}

bool FOwnedAbilityUpgradeList::FindOwnedAbilityUpgrade(const FGameplayTag& InTag)
{
    for (auto& OwnedAbilityUpgrade : OwnedAbilityUpgrades)
    {
        if (OwnedAbilityUpgrade.UpgradeTag == InTag)
        {
            return true;
        }
    }
    return false;
}

FOwnedAbilityUpgrade* FOwnedAbilityUpgradeList::GetOwnedAbilityUpgrade(const FGameplayTag& InTag)
{
    for (auto& OwnedAbilityUpgrade : OwnedAbilityUpgrades)
    {
        if (OwnedAbilityUpgrade.UpgradeTag == InTag)
        {
            return &OwnedAbilityUpgrade;
        }
    }
    return nullptr;
}

AAuraPlayerState::AAuraPlayerState()
{
    // 서버 업데이트 빈도
    // GAS에 적용하기 위해 빈도를 더 빠르게 조정
    SetNetUpdateFrequency(100.f);

    AbilitySystemComponent = CreateDefaultSubobject<UAuraAbilitySystemComponent>("AbilitySystemComponent");

    // 멀티에서의 복제
    AbilitySystemComponent->SetIsReplicated(true);
    AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

    AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>("AttributeSet");

    // 인벤토리
    Inventory = CreateDefaultSubobject<UInventoryComponent>("Inventory");
    Inventory->SetIsReplicated(true);
    
    Equipment = CreateDefaultSubobject<UEquipmentComponent>("Equipment");
    Equipment->SetIsReplicated(true);

    Charm = CreateDefaultSubobject<UCharmComponent>("Charm");
    Charm->SetIsReplicated(true);
}

void AAuraPlayerState::BeginPlay()
{
    Super::BeginPlay();
    
    // 초기화 완료
    OnPlayerStateInitialized.Broadcast(this);
}

void AAuraPlayerState::CopyProperties(APlayerState* PlayerState)
{
    // 맵 이동 '전' 플레이어 스테이트에 의해 호출됨 - 인자는 새로운 플레이어 스테이트
    Super::CopyProperties(PlayerState);
    
    if (AAuraPlayerState* NewPlayerState = Cast<AAuraPlayerState>(PlayerState))
    {
        // 기존 데이터를 새 플레이어 스테이트의 데이터로 복사
        NewPlayerState->SetLevel(Level);
        NewPlayerState->SetXP(XP);
        NewPlayerState->SetAttributePoints(AttributePoints);
        NewPlayerState->SetSpellPoints(SpellPoints);
        NewPlayerState->SetAbilityUpgradeTagContainer(OwnedAbilityUpgradeList);
    
        NewPlayerState->bIsDataLoaded = true;
    }
}

void AAuraPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AAuraPlayerState, Level);
    DOREPLIFETIME(AAuraPlayerState, XP);
    DOREPLIFETIME(AAuraPlayerState, AttributePoints);
    DOREPLIFETIME(AAuraPlayerState, SpellPoints);
    DOREPLIFETIME(AAuraPlayerState, OwnedAbilityUpgradeList);
    DOREPLIFETIME(AAuraPlayerState, ReplicatedCardInfo);
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
    OnLevelChangedDelegate.Broadcast(Level, false);
}

void AAuraPlayerState::AddToLevelOne()
{
    AddToLevel(1);
}

void AAuraPlayerState::AddToLevel(int32 InLevel)
{
    Level += InLevel;
    OnLevelChangedDelegate.Broadcast(Level, true);
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

void AAuraPlayerState::SetHealth(const float InHealth)
{
    if (UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AttributeSet))
    {
        AuraAS->SetHealth(InHealth);
    }
}

void AAuraPlayerState::SetMana(const float InMana)
{
    if (UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AttributeSet))
    {
        AuraAS->SetMana(InMana);
    }
}

void AAuraPlayerState::SetUpgradeCardInfo(const TArray<FAuraAbilityUpgradeInfo>& NewCard)
{
    ReplicatedCardInfo = NewCard;
    OnUpgradeCardsInitializedDelegate.Broadcast(ReplicatedCardInfo);
}

void AAuraPlayerState::SetAbilityUpgradeTagContainer(const FOwnedAbilityUpgradeList& InOwnedAbilityUpgradeList)
{
    OwnedAbilityUpgradeList = InOwnedAbilityUpgradeList;
    OwnedAbilityUpgradeList.MarkArrayDirty();
}

void AAuraPlayerState::ResetAttributesToBaseValue()
{
    for (TFieldIterator<FProperty> It(GetClass()); It; ++It)
    {
        if (FGameplayAttribute::IsGameplayAttributeDataProperty(*It))
        {
            FGameplayAttributeData* AttributeData = It->ContainerPtrToValuePtr<FGameplayAttributeData>(this);
            AttributeData->SetBaseValue(AttributeData->GetCurrentValue());
            AttributeData->SetCurrentValue(AttributeData->GetCurrentValue());
        }
    }
}

void AAuraPlayerState::InitializeDefaultAttributesFromAttributeSet(UAbilitySystemComponent* NewASC,
    UAttributeSet* AS)
{
    // 액터의 클래스 정보 가져오기
	UCharacterClassInfo* CharacterClassInfo = UAuraAbilitySystemLibrary::GetCharacterClassInfo(this);
	if (CharacterClassInfo == nullptr)
		return;
	
	UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AS);
	if (!AuraAS)
		return;

	// Set by Caller 가져오기
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	// 이펙트 컨텍스트 핸들 생성
	FGameplayEffectContextHandle EffectContextHandle = NewASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);

	// 이펙트 적용을 위한 이펙트 스펙 핸들 생성
	const FGameplayEffectSpecHandle SpecHandle = NewASC->MakeOutgoingSpec(CharacterClassInfo->PrimaryAttributes_SetByCaller, 1.f, EffectContextHandle);

	float StrengthMag = AuraAS->GetStrength();
	float IntMag = AuraAS->GetIntelligence();
	float ResMag = AuraAS->GetResilience();
	float Vigor = AuraAS->GetVigor();
	
	// Set By Caller Magnitude 설정
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Strength, StrengthMag);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Intelligence, IntMag);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Resilience, ResMag);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Vigor, Vigor);

	// 게임플레이 이펙트 적용
	NewASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 2차 속성 적용
	FGameplayEffectContextHandle SecondaryAttributesContextHandle = NewASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(this);
	
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = NewASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttributes, 1.f, SecondaryAttributesContextHandle);
	NewASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

	// 바이탈 속성 적용
	FGameplayEffectContextHandle VitalAttributesContextHandle = NewASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(this);
	
	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = NewASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes_SetByCaller, 1.f, VitalAttributesContextHandle);
	
	float Health = AuraAS->GetHealth();
	float Mana = AuraAS->GetMana();
	
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(VitalAttributesSpecHandle, GameplayTags.Attributes_Vital_Health, Health);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(VitalAttributesSpecHandle, GameplayTags.Attributes_Vital_Mana, Mana);

	NewASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data.Get());

	// 리젠 속성 적용
	FGameplayEffectContextHandle RegenAttributesContextHandle = NewASC->MakeEffectContext();
	RegenAttributesContextHandle.AddSourceObject(this);
	
	const FGameplayEffectSpecHandle RegenAttributesSpecHandle = NewASC->MakeOutgoingSpec(CharacterClassInfo->RegenAttributes, 1.f, RegenAttributesContextHandle);

	NewASC->ApplyGameplayEffectSpecToSelf(*RegenAttributesSpecHandle.Data.Get());
}

void AAuraPlayerState::AddUpgradeTag(const FGameplayTag& Tag)
{
    int32 UpgradeStack = 0;
    
    if (FOwnedAbilityUpgrade* OwnedAbilityUpgrade = OwnedAbilityUpgradeList.GetOwnedAbilityUpgrade(Tag))
    {
        // 이미 존재하는 업그레이드
        OwnedAbilityUpgrade->UpgradeStack++;
        UpgradeStack = OwnedAbilityUpgrade->UpgradeStack;
        OnAbilityUpgradeTagsChangedDelegate.Broadcast(Tag, UpgradeStack);
        OwnedAbilityUpgradeList.MarkArrayDirty();
    }
    else
    {
        // 존재하지 않으면 추가
        OwnedAbilityUpgradeList.OwnedAbilityUpgrades.Add(Tag);
        OnAbilityUpgradeTagsChangedDelegate.Broadcast(Tag, 1);
        OwnedAbilityUpgradeList.MarkArrayDirty();
    }
}

void AAuraPlayerState::RemoveUpgradeTag(const FGameplayTag& Tag)
{
    // 태그 제거

    if (FOwnedAbilityUpgrade* OwnedAbilityUpgrade = OwnedAbilityUpgradeList.GetOwnedAbilityUpgrade(Tag))
    {
        int32 UpgradeStack = 0;
        
        // 이미 존재하는 업그레이드
        OwnedAbilityUpgrade->UpgradeStack--;
        UpgradeStack = FMath::Max(0, OwnedAbilityUpgrade->UpgradeStack);
    
        // 업그레이드 스택이 0보다 작으면 업그레이드 제거
        if (UpgradeStack <= 0)
        {
            OwnedAbilityUpgradeList.OwnedAbilityUpgrades.RemoveAll([&Tag](const FOwnedAbilityUpgrade& Upgrade)
            {
                return Upgrade.UpgradeTag.MatchesTagExact(Tag);
            });
        }
        OwnedAbilityUpgradeList.MarkArrayDirty();
        OnAbilityUpgradeTagsChangedDelegate.Broadcast(Tag, UpgradeStack);
    }
}

int32 AAuraPlayerState::GetUpgradeTagCount(FGameplayTag UpgradeTag)
{
    if (!UpgradeTag.IsValid())
        return 0;

    if (auto* Upgrade = OwnedAbilityUpgradeList.GetOwnedAbilityUpgrade(UpgradeTag))
    {
        return Upgrade->UpgradeStack;
    }

    return 0;
}

bool AAuraPlayerState::HasUpgradeTag(FGameplayTag UpgradeTag)
{
    return OwnedAbilityUpgradeList.FindOwnedAbilityUpgrade(UpgradeTag);
}

void AAuraPlayerState::HandleAbilitiesSet()
{
    // 어빌리티 부여 종료 후 호출되는 콜백 함수
    
}

TArray<FGameplayTag> AAuraPlayerState::GetAllAbilityTags()
{
    return FAuraGameplayTags::Get().GameplayAbilitiesTags;
}

TArray<FGameplayTag> AAuraPlayerState::GetAllActiveAbilityTags() const
{
    TArray<FGameplayTag> AllActiveTags;

    auto& AuraTags = FAuraGameplayTags::Get();
    
    // 활성화된 어빌리티
    TArray<FGameplayAbilitySpec> ActiveSpecs = AbilitySystemComponent->GetActivatableAbilities();

    for (FGameplayAbilitySpec& Spec : ActiveSpecs)
    {
        FGameplayTagContainer AbilityTagContainer = Spec.Ability->GetAssetTags();
        if (AbilityTagContainer.HasTag(AuraTags.Abilities_None))
            continue;

        if (Spec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Eligible)
            || Spec.GetDynamicSpecSourceTags().HasTag(FAuraGameplayTags::Get().Abilities_Status_Locked))
            continue;
        
        if (Spec.Level <= 0)
            continue;

        if (AbilityTagContainer.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities.Fire")))
            || AbilityTagContainer.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities.Arcane")))
            || AbilityTagContainer.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities.Lightning")))
            )
        {
            for (auto Tag : AbilityTagContainer)
            {
                if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities"))))
                {
                    if (UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this))
                    {
                        FAuraAbilityInfo* AuraAbilityInfo = AbilityInfo->FindAbilityInfoForTag(Tag);
                        
                        if (AuraAbilityInfo->StatusTag.MatchesTag(AuraTags.Abilities_Status_Eligible)
                           || AuraAbilityInfo->StatusTag.MatchesTag(AuraTags.Abilities_Status_Equipped))
                        {
                            // 어빌리티 태그라면 추가
                            AllActiveTags.Add(Tag);
                        }
                    }
                }
            }
        }
    }
    
    return AllActiveTags;
}

TArray<FGameplayTag> AAuraPlayerState::GetAllInActiveAbilityTags() const
{
    TArray<FGameplayTag> AllTags = FAuraGameplayTags::Get().GameplayAbilitiesTags;
    TArray<FGameplayTag> AllActiveTags;

    auto& AuraTags = FAuraGameplayTags::Get();
    
    // 활성화된 어빌리티
    TArray<FGameplayAbilitySpec> ActiveSpecs = AbilitySystemComponent->GetActivatableAbilities();

    for (FGameplayAbilitySpec& Spec : ActiveSpecs)
    {
        FGameplayTagContainer AbilityTagContainer = Spec.Ability->GetAssetTags();
        if (AbilityTagContainer.HasTag(AuraTags.Abilities_None))
            continue;

        for (auto Tag : AbilityTagContainer)
        {
            if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities"))))
            {
                // 어빌리티 태그라면 추가
                AllActiveTags.Add(Tag);
            }
        }
    }

    // 모든 어빌리티 태그에서 활성화된 태그를 제거
    for (auto ActiveTag : AllActiveTags)
    {
        AllTags.Remove(ActiveTag);
    }

    return AllTags;
}

void AAuraPlayerState::Server_AddAbilityUpgradeTag_Implementation(FGameplayTag UpgradeTag)
{
    // 비 보유 중인 어빌리티라면 새로 습득, 보유 중인 어빌리티라면 레벨 상승
    
    // 어빌리티 군 업그레이드는 Abilties.Fire / Abilities.Lightning / Abilities.Arcane
    // 만약 어빌리티 획득 업그레이드(또는 레벨업 업그레이드)라면 업그레이드 태그로 저장하지 않음
    if (UpgradeTag.RequestDirectParent().MatchesTag(FGameplayTag::RequestGameplayTag("Abilities")))
    {
        if (auto* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
        {
            AuraASC->AddCharacterAbilityByTag(UpgradeTag);
            AuraASC->AbilityStatusChanged.Broadcast(UpgradeTag, FAuraGameplayTags::Get().Abilities_Status_Equipped, 0);
        }
    }
    
    // 보유중인 어빌리티 업그레이드 배열에 추가
    AddUpgradeTag(UpgradeTag);
}

void AAuraPlayerState::Server_RemoveAbilityUpgradeTag_Implementation(FGameplayTag UpgradeTag)
{
    // 어빌리티 획득 업그레이드(또는 레벨업 업그레이드) 제거
    if (UpgradeTag.RequestDirectParent().MatchesTag(FGameplayTag::RequestGameplayTag("Abilities")))
    {
        if (auto* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
        {
            AuraASC->RemoveCharacterAbilityByTag(UpgradeTag, 1);
        }
    }
    
    // 보유중인 어빌리티 업그레이드 배열에서 제거
    RemoveUpgradeTag(UpgradeTag);
}

// void AAuraPlayerState::OnRep_AbilityUpgradeTags()
// {
//     // 업그레이드 목록 UI로 전송
//     OnAbilityUpgradeTagsChangedDelegate.Broadcast(OwnedAbilityUpgradeTags);
// }

void AAuraPlayerState::OnRep_UpgradeCardInfo()
{
    // 카드 정보를 받아 UI로 추가
    OnUpgradeCardsInitializedDelegate.Broadcast(ReplicatedCardInfo);
}

void AAuraPlayerState::OnRep_Level(int32 OldLevel)
{
    OnLevelChangedDelegate.Broadcast(Level, bIsLevelInitialized);
    if (!bIsLevelInitialized)
    {
        // 블루프린트로 전달
        bIsLevelInitialized = true;
    }
}

void AAuraPlayerState::OnRep_XP(int32 OldXP)
{
    // 블루프린트로 전달
    OnXPChangedDelegate.Broadcast(XP);
}

void AAuraPlayerState::OnRep_AttributePoint(int32 OldAttributePoint)
{
    // 사용 가능한 포인트 있음 알리기
    if (OldAttributePoint > 0)
    {
        UAuraAbilitySystemLibrary::AddMessageToActor(
            AbilitySystemComponent->GetAvatarActor(),
            FGameplayTag::RequestGameplayTag("Message.LevelUp"));
    }
    else
    {
        UAuraAbilitySystemLibrary::RemoveMessageTagEffectToSelf(
            AbilitySystemComponent,
            FGameplayTag::RequestGameplayTag("Message.LevelUp"));
    }
    
    //
    OnAttributePointChangedDelegate.Broadcast(AttributePoints);
}

void AAuraPlayerState::OnRep_SpellPoint(int32 OldSpellPoint)
{
    // 사용 가능한 포인트 있음 알리기
    if (OldSpellPoint > 0)
    {
        UAuraAbilitySystemLibrary::AddMessageToActor(
            AbilitySystemComponent->GetAvatarActor(),
            FGameplayTag::RequestGameplayTag("Message.LevelUp"));
    }
    else
    {
        UAuraAbilitySystemLibrary::RemoveMessageTagEffectToSelf(
            AbilitySystemComponent,
            FGameplayTag::RequestGameplayTag("Message.LevelUp"));
    }

    //
    OnSpellPointChangedDelegate.Broadcast(SpellPoints);
}
