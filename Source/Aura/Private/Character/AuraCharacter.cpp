// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Player/AuraPlayerState.h"
#include "Player/AuraPlayerController.h"
#include "UI/HUD/AuraHUD.h"
#include "AbilitySystem/Data/LevelUpInfo.h"
#include "NiagaraComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"
#include "Game/AuraGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LoadScreenSaveGame.h"

AAuraCharacter::AAuraCharacter()
{
    LevelUpNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("LevelUpNiagaraComponent");
    LevelUpNiagaraComponent->SetupAttachment(GetRootComponent());
    LevelUpNiagaraComponent->bAutoActivate = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;

    // bUseControllerRotationPitch = false;
    // bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
    SpringArm->SetUsingAbsoluteRotation(true);
    SpringArm->bDoCollisionTest = false;
    SpringArm->SetupAttachment(RootComponent);

    Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    CharacterClass = ECharacterClass::Elementalist;
}

void AAuraCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);

    GetWorldTimerManager().ClearTimer(DeathTimer);
}

void AAuraCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    // 서버를 위해 어빌리티 액터 정보 초기화
    InitAbilityActorInfo();

    // 저장 데이터 불러오기
    LoadProgress();

    // 저장 월드 상태 불러오기
    if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        AuraGameMode->LoadWorldState(GetWorld());
    }
}

void AAuraCharacter::LoadProgress()
{
    // 게임 모드에 접근
    AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));

    if (AuraGameMode)
    {
        // 저장 슬롯 찾기
        ULoadScreenSaveGame* SaveData = AuraGameMode->RetrieveInGameSaveData();
        if (SaveData == nullptr)
            return;

        // 첫 로딩일 때
        if (SaveData->bFirstTimeLoading)
        {
            // 기본 1차 속성 적용
            InitializeDefaultAttributes();
            AddCharacterAbilites();
        }
        else
        {
            UAuraAbilitySystemComponent* AuraASC = CastChecked<UAuraAbilitySystemComponent>(AbilitySystemComponent);
            
            // 서버에서만 실행
            if (HasAuthority() == false)
                return;
            
            // 저장된 세이브에서 어빌리티 불러오기
            AuraASC->AddCharacterAbilitiesFromSaveData(SaveData);
            
            // 저장된 데이터 불러오기
            if (AAuraPlayerState* AuraPlayerState = Cast<AAuraPlayerState>(GetPlayerState()))
            {
                AuraPlayerState->SetLevel(SaveData->PlayerLevel);
                AuraPlayerState->SetXP(SaveData->XP);
                AuraPlayerState->SetAttributePoints(SaveData->AttributePoints);
                AuraPlayerState->SetSpellPoints(SaveData->SpellPoints);
            }
            
            // 1차 속성, 2차 속성 적용
            UAuraAbilitySystemLibrary::InitializeDefaultAttributesFromSaveData(this, AbilitySystemComponent, SaveData);
        }
    }
}

void AAuraCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    // 클라이언트를 위해 어빌리티 액터 정보 초기화
    InitAbilityActorInfo();
}

void AAuraCharacter::AddToXP_Implementation(int32 InXP)
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    AuraPlayerState->AddToXP(InXP);
}

void AAuraCharacter::LevelUp_Implementation()
{
    MulticastLevelUpParticles();
}

void AAuraCharacter::MulticastLevelUpParticles_Implementation() const
{
    if (IsValid(LevelUpNiagaraComponent))
    {
        // 나이아가라 시스템을 카메라 방향으로 정렬
        const FVector CameraLocation = Camera->GetComponentLocation();
        const FVector NiagaraSystemLocation = LevelUpNiagaraComponent->GetComponentLocation();
        const FRotator ToCameraRotation = (CameraLocation - NiagaraSystemLocation).Rotation();
        LevelUpNiagaraComponent->SetWorldRotation(ToCameraRotation);

        LevelUpNiagaraComponent->Activate(true);
    }
}

int32 AAuraCharacter::GetXP_Implementation() const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    return AuraPlayerState->GetXP();
}

int32 AAuraCharacter::FindLevelForXP_Implementation(int32 InXP) const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    return AuraPlayerState->LevelUpInfo->FindLevelForXP(InXP);
}

int32 AAuraCharacter::GetAttributePointsReward_Implementation(int32 Level) const
{
    const AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    return AuraPlayerState->LevelUpInfo->LevelUpInformation[Level].AttributePointAward;
}

int32 AAuraCharacter::GetSpellPointsReward_Implementation(int32 Level) const
{
    const AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    return AuraPlayerState->LevelUpInfo->LevelUpInformation[Level].SpellPointAward;
}

void AAuraCharacter::AddToPlayerLevel_Implementation(int32 InLevel)
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    AuraPlayerState->AddToLevel(InLevel);

    if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        AuraASC->UpdateAbilityStatus(AuraPlayerState->GetCharacterLevel());
    }
}

void AAuraCharacter::AddToAttributePoints_Implementation(int32 InAttributePoints)
{
    // TODO : 속성 포인트 PlayerState에 구현
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    AuraPlayerState->AddToAttributePoints(InAttributePoints);
}

void AAuraCharacter::AddToSpellPoints_Implementation(int32 InSpellPoints)
{
    // TODO : 속성 포인트 PlayerState에 구현
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    AuraPlayerState->AddToSpellPoints(InSpellPoints);
}

int32 AAuraCharacter::GetAttributePoints_Implementation() const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    return AuraPlayerState->GetAttributePoints();
}

int32 AAuraCharacter::GetSpellPoints_Implementation() const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    return AuraPlayerState->GetSpellPoints();
}

void AAuraCharacter::ShowMagicCircle_Implementation(UMaterialInterface* DecalMaterial) const
{
    if (IsLocallyControlled() == false)
        return;

    AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetController());
    if (AuraPC)
    {
        AuraPC->ShowMagicCircle(DecalMaterial);
        AuraPC->bShowMouseCursor = false;
    }
}

void AAuraCharacter::HideMagicCircle_Implementation() const
{
    if (IsLocallyControlled() == false)
        return;

    AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetController());
    if (AuraPC)
    {
        AuraPC->HideMagicCircle();
        AuraPC->bShowMouseCursor = true;
    }
}

void AAuraCharacter::SaveProgress_Implementation(const FName& CheckpointTag)
{
    // 게임 모드에 접근
    AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
    
    if (AuraGameMode)
    {
        // 저장 슬롯 찾기
        ULoadScreenSaveGame* SaveData = AuraGameMode->RetrieveInGameSaveData();
        if (SaveData == nullptr)
            return;

        // 데이터 저장
        SaveData->PlayerStartTag = CheckpointTag;

        if (AAuraPlayerState* AuraPlayerState = Cast<AAuraPlayerState>(GetPlayerState()))
        {
            SaveData->PlayerLevel = AuraPlayerState->GetCharacterLevel();
            SaveData->XP = AuraPlayerState->GetXP();
            SaveData->AttributePoints = AuraPlayerState->GetAttributePoints();
            SaveData->SpellPoints = AuraPlayerState->GetSpellPoints();
        }

        // 1차 속성
        SaveData->Strength = UAuraAttributeSet::GetStrengthAttribute().GetNumericValue(GetAttributeSet());
        SaveData->Intelligence = UAuraAttributeSet::GetIntelligenceAttribute().GetNumericValue(GetAttributeSet());
        SaveData->Resilience = UAuraAttributeSet::GetResilienceAttribute().GetNumericValue(GetAttributeSet());
        SaveData->Vigor = UAuraAttributeSet::GetVigorAttribute().GetNumericValue(GetAttributeSet());

        // 바이탈 속성
        SaveData->Health = UAuraAttributeSet::GetHealthAttribute().GetNumericValue(GetAttributeSet());
        SaveData->Mana = UAuraAttributeSet::GetHealthAttribute().GetNumericValue(GetAttributeSet());
        
        SaveData->bFirstTimeLoading = false;

        if (HasAuthority() == false)
            return;
        
        // 델리게이트 생성 및 바인딩
        UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent);
        
        FForEachAbility SaveAbilityDelegate;
        SaveData->SavedAbilities.Empty();
        SaveAbilityDelegate.BindLambda([this, AuraASC, SaveData](const FGameplayAbilitySpec& AbilitySpec)
        {
            // 어빌리티 스펙에서 태그 가져오기
            const FGameplayTag AbilityTag = AuraASC->GetAbilityTagFromSpec(AbilitySpec);

            // 어빌리티 정보 가져오기
            UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this);

            // 어빌리티 태그에 해당하는 어빌리티 정보 가져오기
            FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
            
            FSavedAbility SavedAbility;
            SavedAbility.GameplayAbility = Info.Ability;
            SavedAbility.AbilityTag = AbilityTag;
            SavedAbility.AbilityLevel = AbilitySpec.Level;
            SavedAbility.AbilitySlot = AuraASC->GetSlotFromAbilityTag(AbilityTag);
            SavedAbility.AbilityStatus = AuraASC->GetStatusFromAbilityTag(AbilityTag);
            SavedAbility.AbilityType = Info.AbilityType;

            // 어빌리티 저장
            SaveData->SavedAbilities.AddUnique(SavedAbility);
        });
        
        // 활성화된 어빌리티에 대하여 델리게이트 호출
        AuraASC->ForEachAbility(SaveAbilityDelegate);
        
        AuraGameMode->SaveInGameProgressData(SaveData);
    }
}

int32 AAuraCharacter::GetCharacterLevel_Implementation()
{
    const AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);

    return AuraPlayerState->GetCharacterLevel();
}

void AAuraCharacter::Die(const FVector& DeathImpulse)
{
    // 랙돌 효과 발생
    Super::Die(DeathImpulse);

    FTimerDelegate DeathTimerDelegate;
    DeathTimerDelegate.BindLambda([this]()
    {
        AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this));
        if (IsValid(AuraGM))
        {
            AuraGM->PlayerDied(this);
        }
    });
    
    // 타이머 설정
    GetWorldTimerManager().SetTimer(DeathTimer, DeathTimerDelegate, DeathTime, false);

    // 카메라 추락 방지
    Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
}

void AAuraCharacter::ShowDamageNumber_Implementation(float Damage, bool bBlocked, bool bCriticalHit, bool bHealed)
{
    if (AAuraPlayerController* AuraPlayerController = Cast<AAuraPlayerController>(GetController()))
    {
        AuraPlayerController->ShowDamageNumber(Damage, this, bBlocked, bCriticalHit, bHealed);
    }
}

void AAuraCharacter::OnRep_Stunned()
{
    if (auto* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent))
    {
        const auto& GameplayTags = FAuraGameplayTags::Get();

        FGameplayTagContainer BlockedTags;
        BlockedTags.AddTag(GameplayTags.Player_Block_CursorTrace);
        BlockedTags.AddTag(GameplayTags.Player_Block_InputHeld);
        BlockedTags.AddTag(GameplayTags.Player_Block_InputPressed);
        BlockedTags.AddTag(GameplayTags.Player_Block_InputReleased);

        if (bIsStunned)
        {
            AuraASC->AddLooseGameplayTags(BlockedTags);
            StunDebuffComponent->Activate();
        }
        else
        {
            AuraASC->RemoveLooseGameplayTags(BlockedTags);
            StunDebuffComponent->Deactivate();
        }
    }
}

void AAuraCharacter::OnRep_Burned()
{
    if (bIsBurned)
    {
        BurnDebuffComponent->Activate();
    }
    else
    {
        BurnDebuffComponent->Deactivate();
    }
}

void AAuraCharacter::InitAbilityActorInfo()
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);
    
    // 소유자 액터, 아바타 액터
    AuraPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(AuraPlayerState, this);
    
    // Actor Info가 세팅되었음을 알림
    Cast<UAuraAbilitySystemComponent>(AuraPlayerState->GetAbilitySystemComponent())->AbilityActorInfoSet();

    // ASC와 특성 세트를 가져와서 할당
    AbilitySystemComponent = AuraPlayerState->GetAbilitySystemComponent();
    AttributeSet = AuraPlayerState->GetAttributeSet();

    // ASC 생성 완료를 알리는 델리게이트 호출
    OnASCRegistered.Broadcast(AbilitySystemComponent);

    // 스턴 태그 대기
    AbilitySystemComponent->RegisterGameplayTagEvent(FAuraGameplayTags::Get().Debuff_Stun, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AAuraCharacter::StunTagChanged);

    // 이 클라이언트가 조작하는 컨트롤러가 아니면 null로 표시됨 -> null check 필요
    if (AAuraPlayerController* AuraPlayerController = Cast<AAuraPlayerController>(GetController()))
    {
        if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(AuraPlayerController->GetHUD()))
        {
            AuraHUD->InitOverlay(AuraPlayerController, AuraPlayerState, AbilitySystemComponent, AttributeSet);
        }
    }
}
