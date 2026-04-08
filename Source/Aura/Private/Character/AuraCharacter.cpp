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
#include "EnhancedInputSubsystems.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Game/AuraGameInstance.h"
#include "Game/AuraGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Game/LoadScreenSaveGame.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UI/WidgetController/OverlayWidgetController.h"

AAuraCharacter::AAuraCharacter()
{
    bReplicates = true;
    LevelUpNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>("LevelUpNiagaraComponent");
    LevelUpNiagaraComponent->SetupAttachment(GetRootComponent());
    LevelUpNiagaraComponent->bAutoActivate = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 400.f, 0.f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
    SetReplicateMovement(true);
    
    // bUseControllerRotationPitch = false;
    // bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = false;

    SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
    SpringArm->SetUsingAbsoluteRotation(true);
    SpringArm->bDoCollisionTest = false;
    SpringArm->TargetArmLength = 1000.f;
    SpringArm->SetupAttachment(RootComponent);

    // FadeActor용
    Box = CreateDefaultSubobject<UBoxComponent>("Box");
    Box->SetupAttachment(SpringArm);
    Box->SetRelativeScale3D(FVector(10.05f,2.8f,2.8f));
    Box->SetRelativeLocation(FVector(SpringArm->TargetArmLength, 0, 0));

    Camera = CreateDefaultSubobject<UCameraComponent>("Camera");
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
    Camera->bUsePawnControlRotation = false;

    MiniMapCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("MiniMapCapture");
    MiniMapCapture->SetupAttachment(RootComponent);
    MiniMapCapture->SetRelativeLocation(FVector(0.f, 0.f, 1000.f));
    MiniMapCapture->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
    MiniMapCapture->bCaptureEveryFrame = false;
    MiniMapCapture->bCaptureOnMovement = true;
    MiniMapCapture->PrimaryComponentTick.bCanEverTick = true;
    
    CharacterClass = ECharacterClass::Elementalist;
}

void AAuraCharacter::BeginPlay()
{
    Super::BeginPlay();
    
    if (const UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AttributeSet))
    {
        GetCharacterMovement()->MaxWalkSpeed = AuraAS->GetMovementSpeed();
    }
    
    if (APawn* Pawn = Cast<APawn>(this))
    {
        if (AAuraPlayerController* AuraPC = Pawn->GetController<AAuraPlayerController>())
            AuraPC->OnCharacterInit.Broadcast(this);
    }
}

void AAuraCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 미니맵 캡쳐 해제
    GetWorldTimerManager().ClearTimer(MiniMapUpdateTimerHandle);
    GetWorldTimerManager().ClearTimer(DeathTimer);
    
    Super::EndPlay(EndPlayReason);
}

void AAuraCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    InitAbilityActorInfo();
    InitializeDefaultAttributes();
        
    // 기본 어빌리티 부여
    AddCharacterAbilites();
    
    // 리슨 서버라면
    if (HasAuthority())
    {
        AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>();
        if (!AuraPS)
            return;
        
        // 부활
        if (AuraPS->GetAbilitySystemComponent()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead"))))
        {
            // IMC 해제
            if (auto AuraPC = Cast<AAuraPlayerController>(NewController))
            {
                AuraPC->AddAllMappingContexts();
            }
            
            // 사망 태그 제거
            if (UAuraGameInstance* AuraGI = AuraPS->GetGameInstance<UAuraGameInstance>())
            {
                TSubclassOf<UGameplayEffect> DeadTagEffectClass = AuraGI->DeadTagEffectClass;
                AuraPS->GetAbilitySystemComponent()->RemoveActiveGameplayEffectBySourceEffect(DeadTagEffectClass, AuraPS->GetAbilitySystemComponent());
                // LoadProgressFromSaveGame(NewController);
                return;
            }
        }
        
        if (NewController->IsLocalController())
        {
            LoadProgressFromSaveGame(NewController);
        }
        
        if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
        {
            AuraGameMode->LoadWorldState(GetWorld());
        }
        
        if (IsLocallyControlled())
        {
            InitializeMiniMap();
        
            if (MiniMapRenderTarget)
            {
                if (UOverlayWidgetController* OverlayWC = UAuraAbilitySystemLibrary::GetOverlayWidgetController(this))
                {
                    OverlayWC->OnRenderTargetCreated.Broadcast(MiniMapRenderTarget);
                }
            }
        }
    }
}

void AAuraCharacter::LoadProgressFromSaveGame(AController* PC)
{
    if (!HasAuthority())
        return;
    
    // 게임 인스턴스에 접근
    if (UAuraGameInstance* AuraGI = PC->GetGameInstance<UAuraGameInstance>())
    {
        // 저장 슬롯 찾기
        const FString InGameLoadSlotName = AuraGI->LoadSlotName;
        const int32 InGameLoadSlotIndex = AuraGI->LoadSlotIndex;

        ULoadScreenSaveGame* SaveData = AuraGI->GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
        if (SaveData == nullptr)
            return;

        UAuraAbilitySystemComponent* AuraASC = CastChecked<UAuraAbilitySystemComponent>(AbilitySystemComponent);
        if (!AuraASC)
            return;
            
        // 첫 로딩일 때
        if (SaveData->bFirstTimeLoading)
        {
            if (AuraASC)
            {
                // 기본 1차 속성 적용
                InitializeDefaultAttributes();
                SaveData->bFirstTimeLoading = false;
            }
        }
        else
        {
            // 저장된 세이브에서 어빌리티 불러오기
            // AuraASC->AddCharacterAbilitiesFromSaveData(SaveData);
            Client_LoadProgressFromSaveGame();
            
            // 저장된 데이터 불러오기
            // if (AAuraPlayerState* AuraPlayerState = Cast<AAuraPlayerState>(GetPlayerState()))
            // {
            //     AuraPlayerState->SetLevel(SaveData->PlayerLevel);
            //     AuraPlayerState->SetXP(SaveData->XP);
            //     AuraPlayerState->SetAttributePoints(SaveData->AttributePoints);
            //     AuraPlayerState->SetSpellPoints(SaveData->SpellPoints);
            //     AuraPlayerState->SetAbilityUpgradeTagContainer(SaveData->SavedAbilityUpgradeList);
            //     
            //     // 인벤토리 불러오기
            //     UAuraAbilitySystemLibrary::GetInventoryComponentByPlayerState(AuraPlayerState)->SetInventorySlots(SaveData->SavedInventorySlots);
            //     UAuraAbilitySystemLibrary::GetEquipmentComponentByPlayerState(AuraPlayerState)->SetEquipmentSlots(SaveData->SavedEquipmentSlots);
            // }
            //
            // // 기존에 적용된 이펙트 제거 후 재적용
            // AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(DefaultPrimaryAttributes, AbilitySystemComponent);
            // AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(DefaultSecondaryAttributes, AbilitySystemComponent);
            // AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(InitializeVitalAttributes, AbilitySystemComponent);
            // AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(InitializeRegenAttributes, AbilitySystemComponent);
            //
            // // 1차 속성, 2차 속성 적용
            // UAuraAbilitySystemLibrary::InitializeDefaultAttributesFromSaveData(this, AbilitySystemComponent, SaveData);
        }
    }
}

void AAuraCharacter::LoadAbilitiesFromSaveGame()
{
    if (!HasAuthority())
        return;
    
    // 게임 인스턴스에 접근
    if (UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>())
    {
        // 저장 슬롯 찾기
        const FString InGameLoadSlotName = AuraGI->LoadSlotName;
        const int32 InGameLoadSlotIndex = AuraGI->LoadSlotIndex;

        ULoadScreenSaveGame* SaveData = AuraGI->GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
        if (SaveData == nullptr)
            return;
        
        UAuraAbilitySystemComponent* AuraASC = CastChecked<UAuraAbilitySystemComponent>(AbilitySystemComponent);
        if (!AuraASC)
            return;
        
        // 시작 어빌리티 부여
        AddCharacterAbilites();
        
        // 저장된 세이브에서 어빌리티 불러오기
        AuraASC->AddCharacterAbilitiesFromSaveData(SaveData);
    }
}

void AAuraCharacter::Server_ApplyClientStat_Implementation(float Level, float XP, int32 SpellPoints,
    int32 AttributePoints)
{
    if (AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>())
    {
        AuraPS->SetLevel(Level);
        AuraPS->SetXP(XP);
        AuraPS->SetSpellPoints(SpellPoints);
        AuraPS->SetAttributePoints(AttributePoints);
    }
}

void AAuraCharacter::Server_ApplyClientAttributes_Implementation(float Strength, float Intelligence, float Vigor,
    float Resilience, float Health, float Mana)
{
    UAuraAbilitySystemLibrary::InitializeDefaultAttributesFromAttributes(this, GetAbilitySystemComponent(), Strength, Intelligence, Vigor, Resilience, Health, Mana);
}

void AAuraCharacter::Server_ApplyClientSavedAbilities_Implementation(const TArray<FSavedAbility>& SavedAbilities,
    const FOwnedAbilityUpgradeList& SavedAbilityUpgrades)
{
    AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>();
    if (!AuraPS)
        return;

    // 어빌리티 부여
    UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AuraPS->GetAbilitySystemComponent());
    if (AuraASC)
    {
        AuraASC->AddCharacterAbilitiesFromArray(SavedAbilities);
    }
    
    // 어빌리티 업그레이드 적용
    AuraPS->SetAbilityUpgradeTagContainer(SavedAbilityUpgrades);
}

void AAuraCharacter::Server_ApplyClientInventory_Implementation(const TArray<FInventorySlot>& InventorySlots,
    const FEquipmentSlotList& EquipmentSlots)
{
    AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>();
    if (!AuraPS)
        return;
    
    UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AuraPS->GetAbilitySystemComponent());
    if (AuraASC)
    {
        AuraPS->GetInventoryComponent()->SetInventorySlots(InventorySlots);
        AuraPS->GetEquipmentComponent()->SetEquipmentSlots(EquipmentSlots);
    }
}

void AAuraCharacter::Client_LoadProgressFromSaveGame_Implementation()
{
    // 게임 인스턴스에 접근
    if (UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>())
    {
        // 저장 슬롯 찾기
        const FString InGameLoadSlotName = AuraGI->LoadSlotName;
        const int32 InGameLoadSlotIndex = AuraGI->LoadSlotIndex;

        ULoadScreenSaveGame* SaveData = AuraGI->GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
        if (SaveData == nullptr)
            return;
        
        // 첫(빈) 세이브 로딩
        if (SaveData->bFirstTimeLoading)
        {
            SaveData->bFirstTimeLoading = false;
            InitializeDefaultAttributes();
            return;
        }
        
        // 기존 세이브 로딩
        Server_ApplyClientStat(SaveData->PlayerLevel, SaveData->XP, SaveData->SpellPoints, SaveData->AttributePoints);
        Server_ApplyClientAttributes(SaveData->Strength, SaveData->Intelligence, SaveData->Vigor, SaveData->Resilience, SaveData->Health, SaveData->Mana);
        Server_ApplyClientSavedAbilities(SaveData->SavedAbilities, SaveData->SavedAbilityUpgradeList);
        Server_ApplyClientInventory(SaveData->SavedInventorySlots, SaveData->SavedEquipmentSlots);
    }
}

void AAuraCharacter::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();

    InitAbilityActorInfo();
    
    //어빌리티 액터 정보 초기화
    if (IsLocallyControlled())
    {
        Client_LoadProgressFromSaveGame();
    }
}

void AAuraCharacter::InitializeMiniMap()
{
    if (IsLocallyControlled())
    {
        // 내 미니맵만 켜기
        if (!MiniMapCapture)
            return;
    
        MiniMapRenderTarget = NewObject<UTextureRenderTarget2D>(this, MiniMapRenderTargetClass);
        if (MiniMapRenderTarget)
        {
            MiniMapRenderTarget->SizeX = 512;
            MiniMapRenderTarget->SizeY = 512;
                 
            MiniMapRenderTarget->UpdateResource();
                
            // 타이머로 수동 캡쳐
            MiniMapCapture->TextureTarget = MiniMapRenderTarget;
            MiniMapCapture->bCaptureEveryFrame = false;
            MiniMapCapture->bCaptureOnMovement = false;
        
            GetWorldTimerManager().SetTimer(
                MiniMapUpdateTimerHandle, 
                this, 
                &AAuraCharacter::UpdateMiniMap, 
                MiniMapUpdateInterval, 
                true
            );
        }
    }
}

void AAuraCharacter::OnRep_Controller()
{
    Super::OnRep_Controller();
    
    if (IsLocallyControlled())
    {
        InitializeMiniMap();
        
        if (MiniMapRenderTarget)
        {
            if (UOverlayWidgetController* OverlayWC = UAuraAbilitySystemLibrary::GetOverlayWidgetController(this))
            {
                OverlayWC->OnRenderTargetCreated.Broadcast(MiniMapRenderTarget);
            }
        }
    }
}

void AAuraCharacter::AddToXP_Implementation(int32 InXP)
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        AuraPlayerState->AddToXP(InXP);
}

void AAuraCharacter::LevelUp_Implementation()
{
    // 레벨업 효과 발생
    MulticastLevelUpParticles();
    
    // 레벨업으로 인한 2차 속성 재계산
    AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(DefaultSecondaryAttributes, AbilitySystemComponent);
    ApplyEffectToSelf(DefaultSecondaryAttributes, 1.f);
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
    if (AuraPlayerState)
        return AuraPlayerState->GetXP();
    
    return 0;
}

int32 AAuraCharacter::FindLevelForXP_Implementation(int32 InXP) const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        return AuraPlayerState->LevelUpInfo->FindLevelForXP(InXP);
    
    return 0;
}

int32 AAuraCharacter::FindXPForLevel_Implementation(int32 InLevel) const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        return AuraPlayerState->LevelUpInfo->FindXPForLevel(InLevel) - AuraPlayerState->GetXP();
    
    return 0;
}

int32 AAuraCharacter::GetAttributePointsReward_Implementation(int32 Level) const
{
    const AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        return AuraPlayerState->LevelUpInfo->LevelUpInformation[Level].AttributePointAward;
    
    return 0;
}

int32 AAuraCharacter::GetSpellPointsReward_Implementation(int32 Level) const
{
    const AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        return AuraPlayerState->LevelUpInfo->LevelUpInformation[Level].SpellPointAward;
    
    return 0;
}

void AAuraCharacter::AddToPlayerLevel_Implementation(int32 InLevel)
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
    {
        if (AuraPlayerState)
        {
            AuraPlayerState->AddToLevel(InLevel);
            AuraASC->UpdateAbilityStatus(AuraPlayerState->GetCharacterLevel());
        }
    }
}

void AAuraCharacter::AddToAttributePoints_Implementation(int32 InAttributePoints)
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        AuraPlayerState->AddToAttributePoints(InAttributePoints);
}

void AAuraCharacter::AddToSpellPoints_Implementation(int32 InSpellPoints)
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        AuraPlayerState->AddToSpellPoints(InSpellPoints);
}

int32 AAuraCharacter::GetAttributePoints_Implementation() const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        return AuraPlayerState->GetAttributePoints();
    
    return 0;
}

int32 AAuraCharacter::GetSpellPoints_Implementation() const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        return AuraPlayerState->GetSpellPoints();
    
    return 0;
}

void AAuraCharacter::SetSpellPoints_Implementation(int32 InPoints) const
{
    AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    if (AuraPlayerState)
        return AuraPlayerState->SetSpellPoints(InPoints);
}

void AAuraCharacter::ShowMagicCircle_Implementation(UMaterialInterface* DecalMaterial, float InRange , float InRadius) const
{
    if (IsLocallyControlled() == false)
        return;

    AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetController());
    if (AuraPC)
    {
        AuraPC->ShowMagicCircle(DecalMaterial, InRange, InRadius);
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
    // 저장중 위젯 생성
    if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetController()))
    {
        if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(AuraPC->GetHUD()))
        {
            AuraHUD->CreateSaveProgressWidget();
        }
    }
    
    // 게임 인스턴스에 접근
    if (UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>())
    {
       const FString InGameLoadSlotName = AuraGI->LoadSlotName;
       const int32 InGameLoadSlotIndex = AuraGI->LoadSlotIndex;

       ULoadScreenSaveGame* SaveData = AuraGI->GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
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
        UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AbilitySystemComponent);
        
        SaveData->Strength = AuraASC->GetNumericAttributeBase(UAuraAttributeSet::GetStrengthAttribute());
        SaveData->Intelligence = AuraASC->GetNumericAttributeBase(UAuraAttributeSet::GetIntelligenceAttribute());
        SaveData->Resilience = AuraASC->GetNumericAttributeBase(UAuraAttributeSet::GetResilienceAttribute());
        SaveData->Vigor = AuraASC->GetNumericAttributeBase(UAuraAttributeSet::GetVigorAttribute());

        // 바이탈 속성, 비율로 저장
        
        float MaxHealth = FMath::Max(1.0f, UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth));
        float Health = AuraASC->GetNumericAttributeBase(UAuraAttributeSet::GetHealthAttribute());
        float MaxMana = FMath::Max(1.0f, UAuraAbilitySystemLibrary::GetAttributeValue(AuraASC, FAuraGameplayTags::Get().Attributes_Secondary_MaxMana));
        float Mana = AuraASC->GetNumericAttributeBase(UAuraAttributeSet::GetManaAttribute());
        
        SaveData->Health = Health / MaxHealth;
        SaveData->Mana = Mana / MaxMana;
        
        SaveData->bFirstTimeLoading = false;
        
        // 델리게이트 생성 및 바인딩
        FForEachAbility SaveAbilityDelegate;
        SaveData->SavedAbilities.Empty();
        SaveAbilityDelegate.BindLambda([this, AuraASC, SaveData](const FGameplayAbilitySpec& AbilitySpec)
        {
            // 어빌리티 스펙에서 태그 가져오기
            const FGameplayTag AbilityTag = AuraASC->GetAbilityTagFromSpec(AbilitySpec);

            // 어빌리티 정보 가져오기
            UAbilityInfo* AbilityInfo = UAuraAbilitySystemLibrary::GetAbilityInfo(this);

            // 어빌리티 태그에 해당하는 어빌리티 정보 가져오기
            FAuraAbilityInfo* Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
            
            FSavedAbility SavedAbility;
            SavedAbility.GameplayAbility = Info->Ability;
            SavedAbility.AbilityTag = AbilityTag;
            SavedAbility.AbilityLevel = AbilitySpec.Level;
            SavedAbility.AbilitySlot = AuraASC->GetSlotFromAbilityTag(AbilityTag);
            SavedAbility.AbilityStatus = AuraASC->GetStatusFromAbilityTag(AbilityTag);
            SavedAbility.AbilityType = Info->AbilityType;

            // 어빌리티 저장
            SaveData->SavedAbilities.AddUnique(SavedAbility);
        });
        
        // 활성화된 어빌리티에 대하여 델리게이트 호출
        AuraASC->ForEachAbility(SaveAbilityDelegate);

        // 어빌리티 업그레이드 저장
        if (AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>())
        {
            SaveData->SavedAbilityUpgradeList = AuraPS->GetOwnedAbilityUpgradeList();
        }
        
        // 소환 위치 저장
        AuraGI->PlayerStartTag = SaveData->PlayerStartTag;

        // 인벤토리 저장
        SaveData->SavedInventorySlots = IPlayerInterface::Execute_GetInventoryComponent(this)->GetSlots();
        SaveData->SavedEquipmentSlots = IPlayerInterface::Execute_GetEquipmentComponent(this)->GetSlots();
        
        // 데이터 저장
        UGameplayStatics::SaveGameToSlot(SaveData, InGameLoadSlotName, InGameLoadSlotIndex);
        
        // 저장중 위젯 제거
        // if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetController()))
        // {
        //     if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(AuraPC->GetHUD()))
        //     {
        //         if (AuraHUD->GetSaveProgressWidget())
        //             AuraHUD->RemoveSaveProgressWidget();
        //     }
        // }
    }
}

UInventoryComponent* AAuraCharacter::GetInventoryComponent_Implementation()
{
    if (AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>())
    {
        return AuraPS->GetInventoryComponent();
    }
    return nullptr;
}

UEquipmentComponent* AAuraCharacter::GetEquipmentComponent_Implementation()
{
    if (AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>())
    {
        return AuraPS->GetEquipmentComponent();
    }
    return nullptr;
}

UCharmComponent* AAuraCharacter::GetCharmComponent_Implementation()
{
    if (AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>())
    {
        return AuraPS->GetCharmComponent();
    }
    return nullptr;
}

void AAuraCharacter::InitializeDefaultAttributes() const
{
    // 기존에 적용된 이펙트 제거 후 재적용
    AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(DefaultPrimaryAttributes, AbilitySystemComponent);
    AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(DefaultSecondaryAttributes, AbilitySystemComponent);
    AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(InitializeVitalAttributes, AbilitySystemComponent);
    AbilitySystemComponent->RemoveActiveGameplayEffectBySourceEffect(InitializeRegenAttributes, AbilitySystemComponent);
    
    ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
    ApplyEffectToSelf(DefaultSecondaryAttributes, 1.f);
    ApplyEffectToSelf(InitializeVitalAttributes, 1.f);
    ApplyEffectToSelf(InitializeRegenAttributes, 1.f);
}

int32 AAuraCharacter::GetCharacterLevel_Implementation()
{
    const AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>();
    check(AuraPlayerState);

    return AuraPlayerState->GetCharacterLevel();
}

FOnDeath* AAuraCharacter::GetOnDeathDelegate()
{
    return Cast<AAuraPlayerState>(GetPlayerState())->GetOnDeathDelegate();
}

void AAuraCharacter::Die(const FVector& DeathImpulse, AAuraCharacter* KilledBy)
{
    // 랙돌 효과 발생
    Super::Die(DeathImpulse, KilledBy);
    
    // IMC 해제
    if (auto AuraPC = GetController<AAuraPlayerController>())
    {
        AuraPC->RemoveAllMappingContexts();
    }
    
    // 카메라 추락 방지
    Camera->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    MiniMapCapture->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    
    GetOnDeathDelegate()->Broadcast(this);
    
    // 관전
    if (AAuraPlayerController* AuraPlayerController = Cast<AAuraPlayerController>(GetController()))
    {
        AuraPlayerController->Client_ShowGameOverWidget();
        AuraPlayerController->Server_StartSpectating();
    }
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

void AAuraCharacter::OnRep_Invincible()
{
    if (bInvincible)
    {
        InvincibleBuffComponent->Activate();
    }
    else
    {
        InvincibleBuffComponent->Deactivate();
    }
}

void AAuraCharacter::UpdateMiniMap()
{
    if (MiniMapCapture && MiniMapRenderTarget)
    {
        MiniMapCapture->CaptureScene();
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
    
    // // 속도 맞추기
    // if (UAuraAttributeSet* AS = Cast<UAuraAttributeSet>(GetAttributeSet()))
    // {
    //     GetCharacterMovement()->MaxWalkSpeed = AS->GetMovementSpeed();
    // }
}
