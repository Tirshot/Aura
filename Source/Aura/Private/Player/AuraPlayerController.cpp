// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerController.h"
#include "Interaction/EnemyInterface.h"
#include "EnhancedInputSubsystems.h"
#include "Input/AuraInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AIController.h"
#include "Components/SplineComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameFramework/Character.h"
#include "UI/Widget/DamageTextComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Actor/AbilityRangeIndicator.h"
#include "Actor/AuraDropItem.h"
#include "Actor/MagicCircle.h"
#include "Aura/Aura.h"
#include "Character/AuraBossMonster.h"
#include "Character/AuraCharacter.h"
#include "Components/BoxComponent.h"
#include "Game/AuraAudioSubsystem.h"
#include "Game/AuraGameInstance.h"
#include "Game/AuraGameModeBase.h"
#include "Game/AuraGameStateBase.h"
#include "Game/AuraGameUserSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/HighlightInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"
#include "UI/ViewModel/MVVM_CardSelection.h"
#include "UI/ViewModel/MVVM_TutorialDialogue.h"
#include "UI/Widget/LoadScreenWidget.h"
#include "UI/WidgetController/GameOverWidgetController.h"

AAuraPlayerController::AAuraPlayerController()
{
    // 서버에서 발생한 변경 사항을 복제하여 모든 클라이언트로 전송(브로드 캐스팅)
    bReplicates = true;
    
    // 길 찾기 스플라인
    Spline = CreateDefaultSubobject<USplineComponent>("Spline");
}

void AAuraPlayerController::BeginPlay()
{
    Super::BeginPlay();
    
    // IMC가 할당되지 않았다면 중단
    check(AuraContext);

    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    if (Subsystem)
    {
        // IMC, 우선순위
        Subsystem->AddMappingContext(AuraContext, 0);
        
        // 튜토리얼에서 메뉴 단축키 사용 금지
        if (!UAuraAbilitySystemLibrary::IsThisMapTutorial(this))
            Subsystem->AddMappingContext(MenuContext, 0);
        
        // ESC 일시정지 메뉴
        Subsystem->AddMappingContext(PauseContext, 0);
    }

    // 마우스 커서 활성화
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;

    // UI와 입력 상호작용
    FInputModeGameAndUI InputModeData;

    // 마우스 커서가 뷰포트에 갇히지 않도록 설정
    InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputModeData.SetHideCursorDuringCapture(false);
    SetInputMode(InputModeData);

    // 델리게이트 바인딩
    OnReviveTimerEnd.AddDynamic(this, &AAuraPlayerController::Server_ReviveFromPlayerStart);
    
    const FString CurrentLevelName = GetWorld()->GetMapName();

    OnCharacterInit.AddDynamic(this, &AAuraPlayerController::CharacterInitialized);
    // 튜토리얼 레벨에서만 위젯 컨트롤러 생성
    if (CurrentLevelName.Contains(TEXT("Tutorial")))
    {
        // 튜토리얼 뷰 모델 생성
        TutorialDialogueViewModel = NewObject<UMVVM_TutorialDialogue>(this);
        TutorialDialogueViewModel->BlueprintInitialize();
        ShowTutorialUI(true);
    }
    
    // 이미 서버에 보스가 등록되어 있는 상태에서 늦게 생성되었다면 호출
    if (AAuraGameStateBase* GS = Cast<AAuraGameStateBase>(GetWorld()->GetGameState()))
    {
        if (GS->GetBossCharacterArrayLength() > 0)
        {
            BossMonsterBind();
        }
    }
}

void AAuraPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    
    // 인벤토리 뷰모델 초기화
    if (AAuraPlayerState* MyPS = GetPlayerState<AAuraPlayerState>())
    {
        MyPS->GetInventoryComponent()->ForceReplication();
        MyPS->GetEquipmentComponent()->ForceReplication();
        
        HandleInventoryUIInit();
    }
}

void AAuraPlayerController::AcknowledgePossession(APawn* P)
{
    Super::AcknowledgePossession(P);
    
    // 인벤토리 뷰모델 초기화
    if (AAuraPlayerState* MyPS = GetPlayerState<AAuraPlayerState>())
    {
        if (UInventoryComponent* InventoryComponent = UAuraAbilitySystemLibrary::GetInventoryComponentByPlayerState(MyPS))
        {
            if (InventoryComponent->GetSlots().Num() > 0)
            {
                HandleInventoryUIInit();
            }
            else
            {
                InventoryComponent->SlotsReplicated.AddUObject(this, &AAuraPlayerController::HandleInventoryUIInit);
            }
        }
    }
}


void AAuraPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // 커스텀 입력 컴포넌트 유효성 확인
    UAuraInputComponent* AuraInputComponent = CastChecked<UAuraInputComponent>(InputComponent);

    AuraInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Move);
    AuraInputComponent->BindAction(ShiftAction, ETriggerEvent::Started, this, &AAuraPlayerController::ShiftPressed);
    AuraInputComponent->BindAction(ShiftAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShiftReleased);
    AuraInputComponent->BindAction(WheelAction, ETriggerEvent::Triggered, this, &AAuraPlayerController::Zoom);
    AuraInputComponent->BindAction(DebugAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ActivateDebugMode);
    AuraInputComponent->BindAction(AttributeMenuAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShowAttributeMenu);
    AuraInputComponent->BindAction(SpellMenuAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShowSpellMenu);
    AuraInputComponent->BindAction(ESCMenuAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShowESCMenu);
    AuraInputComponent->BindAction(AltAction, ETriggerEvent::Started, this, &AAuraPlayerController::ShowItemTitle);
    AuraInputComponent->BindAction(AltAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShowItemTitle);
    AuraInputComponent->BindAction(InventoryMenuAction, ETriggerEvent::Completed, this, &AAuraPlayerController::ShowInventoryMenu);
    
    // 어빌리티와 입력 액션 바인딩
    AuraInputComponent->BindAbiltyActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    
    if (AAuraCharacterBase* Aura = Cast<AAuraCharacterBase>(GetPawn()))
    {
        if (ICombatInterface::Execute_IsDead(Aura))
            return;
    }

    // 커서 추적
    CursorTrace();

    // 클릭으로 이동
    AutoRun();

    // 범위 지정 데칼
    UpdateMagicCircleLocation();

    //
    UpdateRangeIndicatorRotation();
    
    // 아이템 클릭 후 근접하면 획득, 아이템 획득중이 아닐때만 진입
    if (TargetItem && !bIsPickingUpItem)
    {
        if (AAuraDropItem* DropItem = Cast<AAuraDropItem>(TargetItem))
        {
            FVector AuraLocation = GetPawn()->GetActorLocation();
            FVector ItemLocation = DropItem->GetActorLocation();
            
            // 이미 줍기 시도 중이라면 무시
            if (DropItem->IsPendingKillPending()) 
            {
                TargetItem = nullptr;
                ThisActor = nullptr;
                TargetingStatus = ETargetingStatus::None;
                CachedDestination = ItemLocation;
                return;
            }
            
            float Distance = FVector::Dist(AuraLocation, ItemLocation);
            if (Distance <= 150.f)
            {
                DropItem->SetActorHiddenInGame(true);
                DropItem->SetActorEnableCollision(false);
                
                AAuraDropItem* PickUpItem = DropItem;
                bIsPickingUpItem = true;
                TargetItem = nullptr;
                Server_TryPickUpItem(PickUpItem, this);
                ThisActor = nullptr;
                TargetingStatus = ETargetingStatus::None;
                CachedDestination = ItemLocation;
            }
        }
    }
}

void AAuraPlayerController::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    
    // 인벤토리 뷰모델 초기화
    HandleInventoryUIInit();
}

void AAuraPlayerController::CharacterInitialized(ACharacter* InCharacter)
{
    //
    if (!IsLocalController() || !InCharacter || InCharacter != GetPawn())
        return;
    
    if (!InCharacter->IsLocallyControlled())
        return;
    
    UEquipmentComponent* EquipmentComponent = nullptr;
    UInventoryComponent* InventoryComponent = nullptr;
    if (GetPawn()->Implements<UPlayerInterface>())
    {
        EquipmentComponent = IPlayerInterface::Execute_GetEquipmentComponent(InCharacter);
        InventoryComponent = IPlayerInterface::Execute_GetInventoryComponent(InCharacter);
    }
}

void AAuraPlayerController::AddAllMappingContexts()
{
    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    if (Subsystem)
    {
        // IMC, 우선순위
        Subsystem->AddMappingContext(AuraContext, 0);
        
        // 튜토리얼에서 메뉴 단축키 사용 금지
        if (!UAuraAbilitySystemLibrary::IsThisMapTutorial(this))
            Subsystem->AddMappingContext(MenuContext, 2);
        
        // ESC 일시정지 메뉴
        Subsystem->AddMappingContext(PauseContext, 3);
    }
}

void AAuraPlayerController::RemoveAllMappingContexts()
{
    UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
    if (Subsystem)
    {
        // 사망 등의 상태일때 컨텍스트 제거
        FModifyContextOptions Options;
        Options.bForceImmediately = true;
        Subsystem->RemoveMappingContext(AuraContext, Options);
        Subsystem->RemoveMappingContext(MenuContext, Options);
        Subsystem->RemoveMappingContext(PauseContext, Options);
    }
}

void AAuraPlayerController::ShowDamageNumber_Implementation(float DamageAmount, ACharacter* TargetCharacter, bool bBlockedHit, bool bCriticalHit, bool bHealed)
{
    // 위젯
    // IsValid - 사망이 보류 중일 경우를 포함
    if (IsValid(TargetCharacter) && DamageTextComponentClass && IsLocalController())
    {
        // NewObject로 생성했을 때는 수동으로 RegisterComponent 해야함
        UDamageTextComponent* DamageText = NewObject<UDamageTextComponent>(TargetCharacter, DamageTextComponentClass);
        DamageText->RegisterComponent();
        DamageText->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        DamageText->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        DamageText->SetDamageText(DamageAmount, bBlockedHit, bCriticalHit, bHealed);
    }
}

void AAuraPlayerController::AutoRun()
{
    if (!bAutoRunning)
        return;

    // 길 찾기 이용하여 움직임
    if (APawn* ControlledPawn = GetPawn())
    {
        // 폰에 가장 가까운 스플라인의 벡터
        const FVector LocationOnSpline = Spline->FindLocationClosestToWorldLocation(ControlledPawn->GetActorLocation(), ESplineCoordinateSpace::World);

        // 스플라인의 방향 벡터
        const FVector Direction = Spline->FindDirectionClosestToWorldLocation(LocationOnSpline, ESplineCoordinateSpace::World);
        ControlledPawn->AddMovementInput(Direction);

        // 스플라인에서 목적지까지의 벡터
        const float DistanceToDestination = (LocationOnSpline - CachedDestination).Length();

        // 자동 이동 허용 범위에 도달 시 자동 이동 중단
        if (DistanceToDestination <= AutoRunAcceptanceRadius)
        {
            bAutoRunning = false;
            StopAutoRun();
        }
    }
}

void AAuraPlayerController::SetMoveToMouse(bool bAllow)
{
    bAllowMoveToMouse = bAllow;
    if (auto AuraGI = GetGameInstance<UAuraGameInstance>())
    {
        AuraGI->bSavedAllowMoveToMouse = bAllow;
    }
    
    UAuraGameUserSettings::GetAuraGameUserSettings()->SetMoveToMouse(bAllow);
}

void AAuraPlayerController::AutoRunToLocation(const FVector& Location)
{
    if (!GetPawn())
        return;

    CachedDestination = Location;
    bAutoRunning = true;
    Spline->ClearSplinePoints();

    UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, GetPawn()->GetActorLocation(), Location);

    if (NavigationPath && NavigationPath->PathPoints.Num() > 0)
    {
        for (FVector& Point : NavigationPath->PathPoints)
        {
            // 캡슐 높이만큼 보정
            Point += FVector(0.f, 0.f, GetPawn()->GetSimpleCollisionHalfHeight());
            Spline->AddSplinePoint(Point, ESplineCoordinateSpace::World, true);
        }
    }
}

void AAuraPlayerController::AutoRunToActor(AActor* Actor)
{
    if (!GetPawn())
        return;

    CachedDestination = Actor->GetActorLocation();
    bAutoRunning = true;
    Spline->ClearSplinePoints();

    UNavigationPath* NavigationPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetPawn()->GetActorLocation(), Actor);

    if (NavigationPath && NavigationPath->PathPoints.Num() > 0)
    {
        for (FVector& Point : NavigationPath->PathPoints)
        {
            // 캡슐 높이만큼 보정
            Point += FVector(0.f, 0.f, GetPawn()->GetSimpleCollisionHalfHeight());
            Spline->AddSplinePoint(Point, ESplineCoordinateSpace::World, true);
        }
    }
}

void AAuraPlayerController::StopAutoRun()
{
    bAutoRunning = false;
    
    APawn* ControlledPawn = GetPawn();
    if (!IsValid(ControlledPawn))
        return;

    // 눌려 있던 모든 키의 상태를 강제 리셋
    FlushPressedKeys();
        
    // 속도와 가속도 강제 초기화
    if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
    {
        if (UCharacterMovementComponent* MoveComp = ControlledCharacter->GetCharacterMovement())
        {
            MoveComp->StopMovementImmediately(); 
        }
    }
    else
    {
        StopMovement(); 
    }
    
    CachedDestination = ControlledPawn->GetActorLocation();
    ControlledPawn->AddMovementInput(FVector::ZeroVector);

    // Server_StopAutoRun();
}

void AAuraPlayerController::BossMonsterBind()
{
    // 게임 모드의 보스 배열에 접근하여 몽타주 이벤트에 바인딩
    if (AAuraGameStateBase* AuraGS = GetWorld()->GetGameState<AAuraGameStateBase>())
    {
        auto BossArray = AuraGS->GetBossCharactersArray();
        for (const auto Boss : BossArray)
        {
            if (!Boss.Get()->OnBossEventStart.IsAlreadyBound(this, &AAuraPlayerController::Client_OnBossEventStart))
                Boss.Get()->OnBossEventStart.AddDynamic(this, &AAuraPlayerController::Client_OnBossEventStart);
            
            if (!Boss.Get()->OnBossEventEnd.IsAlreadyBound(this, &AAuraPlayerController::Client_OnBossEventEnd))
                Boss.Get()->OnBossEventEnd.AddDynamic(this, &AAuraPlayerController::Client_OnBossEventEnd);
            
            if (!Boss.Get()->OnDeath.IsAlreadyBound(this, &AAuraPlayerController::Client_OnBossDead))
                Boss.Get()->OnDeath.AddDynamic(this, &AAuraPlayerController::Client_OnBossDead);
            
            if (Boss->bIsRoaring)
            {
                Client_OnBossEventStart(Boss.Get()); 
            }
        }
    }
}

void AAuraPlayerController::Client_OnBossEventStart_Implementation(AActor* BossActor)
{
    // 오버레이 감추기, 카메라 전환
    if (AAuraHUD* AuraHUD = GetHUD<AAuraHUD>())
    {
        if (auto BossCharacter = Cast<AAuraBossMonster>(BossActor))
        {
            if (!BossCharacter->bNotCreateHealthBar)
            {
                AuraHUD->CreateBossHealthBarWidget(BossCharacter);
            }
        }
        AuraHUD->HideOverlay();
    }

    // 플레이어 입력 방지
    SetPlayerInputEnable(false);
    
    // 입력 방지 태그 추가
    if (auto AuraGI = GetGameInstance<UAuraGameInstance>())
    {
        if (auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()))
        {
            FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
            FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(AuraGI->InputBlockEffectClass, 1.f, ContextHandle);
            ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
        }
    }
    
    StopAutoRun();
    
    ChangeCameraToBossActor(BossActor, 0.25f, 3.f);
}

void AAuraPlayerController::Client_OnBossEventEnd_Implementation(AActor* BossActor)
{
    // 블렌드 종료 후 오버레이 보이기, 카메라 전환
    if (AAuraHUD* AuraHUD = GetHUD<AAuraHUD>())
    {
        AuraHUD->ShowOverlay();
    }

    // 플레이어 입력 활성화
    SetPlayerInputEnable(true);
    
    // 입력 방지 태그 제거
    if (auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetPawn()))
    {
        FGameplayTagContainer BlockTags;
        BlockTags.AddTag(FAuraGameplayTags::Get().Player_Block_InputHeld);
        BlockTags.AddTag(FAuraGameplayTags::Get().Player_Block_InputPressed);
        BlockTags.AddTag(FAuraGameplayTags::Get().Player_Block_InputReleased);
            
        ASC->RemoveActiveEffectsWithTags(BlockTags);
    }
    
    StopAutoRun();
    
    ChangeCameraToOwn(0.5f);
}

void AAuraPlayerController::Client_OnBossDead_Implementation(AActor* BossActor)
{
    // 오버레이 감추기, 카메라 전환
    if (AAuraHUD* AuraHUD = GetHUD<AAuraHUD>())
    {
        AuraHUD->DestroyBossHealthBarWidget();
        AuraHUD->HideOverlay();
    }
    
    // 음악 재생
    if (auto* AudioSS = GetWorld()->GetSubsystem<UAuraAudioSubsystem>())
    {
        AudioSS->PlayMusicByTag_NoVariable(FGameplayTag::RequestGameplayTag("Sound.Background.Boss.Shaman.Defeated"));
    }
    
    FTimerHandle GlobalTimeDelayHandle;
    
    // 보스 사망 시 딜레이 후 카메라 잠시 전환
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.75f);
    
    SetViewTargetWithBlend(BossActor, 0.8f, VTBlend_Cubic, 0.0f, true);

    GetWorldTimerManager().SetTimer(GlobalTimeDelayHandle, [this]()
    {
        // 오버레이 감추기, 카메라 전환
        if (AAuraHUD* AuraHUD = GetHUD<AAuraHUD>())
        {
            AuraHUD->ShowOverlay();
        }
        
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
        ChangeCameraToOwn(0.f);
    },2.5f,false);
}

void AAuraPlayerController::ChangeCameraToBossActor(AActor* BossActor, float BlendTime, float ReturnTime)
{
    FTimerHandle GlobalTimeDelayHandle;
    // 보스 몬스터 카메라로 전환
    SetViewTargetWithBlend(BossActor, BlendTime,VTBlend_Cubic,0.0f,true);
			
    GetWorldTimerManager().SetTimer(GlobalTimeDelayHandle, [this]()
        {
            UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.f);
        },ReturnTime,false);
}

void AAuraPlayerController::ChangeCameraToOwn(float BlendTime)
{
    SetViewTargetWithBlend(GetPawn(), BlendTime,VTBlend_Cubic,0.0f,true);
}

void AAuraPlayerController::SetPlayerInputEnable(bool bEnable)
{
    if (bEnable)
    {
        // 마우스 커서 활성화
        bShowMouseCursor = true;
        DefaultMouseCursor = EMouseCursor::Default;

        // IMC 해제
        AddAllMappingContexts();
        
        // UI와 입력 상호작용
        FInputModeGameAndUI InputModeData;

        // 마우스 커서가 뷰포트에 갇히지 않도록 설정
        InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputModeData.SetHideCursorDuringCapture(false);
        SetInputMode(InputModeData);
    }
    else
    {
        bShowMouseCursor = false;
        DefaultMouseCursor = EMouseCursor::Default;
        
        // IMC 해제
        RemoveAllMappingContexts();
        
        // UI와 입력 상호작용
        FInputModeUIOnly InputModeData;

        // 마우스 커서가 뷰포트에 갇히지 않도록 설정
        InputModeData.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputModeData);
    }
}

void AAuraPlayerController::ShowTutorialUI(bool bVisibility)
{
    if (!IsValid(TutorialDialogueViewModel))
        return;

    if (bVisibility)
    {
        // 생성 또는 표시
        if (IsValid(TutorialDialogueView))
        {
            TutorialDialogueView->SetVisibility(ESlateVisibility::Visible);
            TutorialDialogueViewModel->SetViewToViewModel(TutorialDialogueView); 
            return;
        }
        
        TutorialDialogueView = CreateWidget<UUserWidget>(this, TutorialDialogueViewClass);
        TutorialDialogueViewModel->SetViewToViewModel(TutorialDialogueView);
        
        TutorialDialogueView->AddToViewport();
    }
    else
    {
        // 제거
        if (IsValid(TutorialDialogueView))
        {
            TutorialDialogueView->RemoveFromParent();
        }
    }
}

void AAuraPlayerController::UpdateMagicCircleLocation()
{
    if (!IsValid(MagicCircle))
        return;
    
    FVector TargetLocation = LastMagicCircleLocation;
    
    // 빈 공간에서 데칼을 근처 위치로 옮김
    if (CursorHit.bBlockingHit)
    {
        TargetLocation = CursorHit.ImpactPoint;
    }
    LastMagicCircleLocation = TargetLocation;
    MagicCircle->SetActorLocation(LastMagicCircleLocation);
}

void AAuraPlayerController::UpdateRangeIndicatorRotation()
{
    if (IsValid(RangeIndicator))
    {
        // 원형이 아닐 때
        if (RangeIndicator->RangeParams.RangeShape != ERangeShape::ERS_Circle)
        {
            if (!CursorHit.bBlockingHit)
                return;
            
            if (AActor* RangeIndicatorOwner = RangeIndicator->Owner)
            {
                FVector StartPoint = RangeIndicatorOwner->GetActorLocation();
                float OwnerCapsuleHeight = RangeIndicatorOwner->GetSimpleCollisionCylinderExtent().Z;
                StartPoint.Z -= OwnerCapsuleHeight;
                
                FVector EndPoint = CursorHit.ImpactPoint;

                FVector Direction = EndPoint - StartPoint;
                Direction.Z = 0.f;

                Direction.Normalize();

                float Length = RangeIndicator->RangeParams.Height;

                FVector NewLocation = StartPoint + (Direction * Length);
                RangeIndicator->SetActorLocation(NewLocation);
                FRotator TargetRotation = Direction.ToOrientationRotator();
                TargetRotation.Pitch = 0.f;
                TargetRotation.Roll = 0.f;

                RangeIndicator->SetActorRotation(TargetRotation);
            }
        }
    }
}

void AAuraPlayerController::HighlightActor(AActor* InActor)
{
    if (!IsValid(GetPawn())) 
        return;
    
    if (!IsValid(InActor) || InActor->IsPendingKillPending()) 
        return;
    
    if (InActor->Implements<UHighlightInterface>())
        IHighlightInterface::Execute_HighlightActor(InActor);
}

void AAuraPlayerController::UnHighlightActor(AActor* InActor)
{
    if (!IsValid(GetPawn())) 
        return;
    
    if (!IsValid(InActor) || InActor->IsPendingKillPending()) 
        return;
    
    if (InActor->Implements<UHighlightInterface>())
        IHighlightInterface::Execute_UnHighlightActor(InActor);
}

bool AAuraPlayerController::GetHitResultUnderMagicCircle(ECollisionChannel TraceChannel, bool bTraceComplex,
    FHitResult& HitResult) const
{
    ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player);
    bool bHit = false;
    if (LocalPlayer && LocalPlayer->ViewportClient)
    {
        FVector MagicCircleLocation;
        if (MagicCircle)
        {
            MagicCircleLocation = MagicCircle->GetActorLocation();
            bHit = GetWorld()->LineTraceSingleByChannel
            (
                HitResult,
                MagicCircleLocation + FVector(0, 0, 500),
                MagicCircleLocation - FVector(0, 0, 500),
                TraceChannel);
        }
        else
        {
            bHit = GetWorld()->LineTraceSingleByChannel(
                HitResult,
                LastMagicCircleLocation + FVector(0, 0, 500),
                LastMagicCircleLocation - FVector(0, 0, 500),
                TraceChannel);
        }
    }

    if(!bHit)
    {
        HitResult = FHitResult();
    }

    return bHit;
}

void AAuraPlayerController::ShowMagicCircle(UMaterialInterface* DecalMaterial, float InRange,  float InRadius)
{
    // 생성
    if (IsValid(MagicCircle) == false)
    {
        AActor* AvatarActor = GetPawn();
        if (!AvatarActor)
            return;
        
        MagicCircle = GetWorld()->SpawnActor<AMagicCircle>(MagicCircleClass);
        MagicCircle->SetCircleRange(InRange);
        MagicCircle->SetDecalSize(InRadius);
        MagicCircle->SetOwner(AvatarActor);
        MagicCircle->CircleInitialized.Broadcast(AvatarActor);
        
        bShowMouseCursor = true;
        
        if (DecalMaterial)
        {
            MagicCircle->SetDecalMaterial(DecalMaterial);
        }
        
        // 서버 RPC 호출
        Server_ApplyWaitForExecuteTag();
    }
}

void AAuraPlayerController::HideMagicCircle()
{
    if (IsValid(MagicCircle))
    {
        AActor* AvatarActor = GetPawn();
        if (!AvatarActor)
            return;
        
        LastMagicCircleLocation = MagicCircle->GetActorLocation();
        MagicCircle->RemoveCircle.Broadcast(AvatarActor);
        MagicCircle->Destroy();
        
        bShowMouseCursor = true;
    }
    GetASC()->RemoveActiveEffectsWithTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Player.Abilities.WaitForExecute")));
}

const FVector AAuraPlayerController::GetMagicCircleLocation()
{
    if (!IsValid(MagicCircle))
    {
        UE_LOG(LogTemp, Warning, TEXT("MagicCircle is InValid, return LastLocation"));
        return LastMagicCircleLocation;
    }
    
    FVector MagicCircleLocation = MagicCircle->GetActorLocation();
    if (UWorld* World = GetWorld())
    {
        FHitResult HitResult;
        FVector HitStart = MagicCircleLocation + FVector(0.f, 0.f, 2000.f);
        FVector HitEnd = MagicCircleLocation + FVector(0.f, 0.f, -2000.f);
        if (World->LineTraceSingleByChannel(HitResult, HitStart, HitEnd, ECC_GroundCheck))
        {
            MagicCircleLocation = HitResult.ImpactPoint;
        }
        else
        {
            MagicCircleLocation = LastMagicCircleLocation;
        }
    }
    LastMagicCircleLocation = MagicCircleLocation;
    return MagicCircleLocation;
}

void AAuraPlayerController::ShowRangeIndicator(ERangeShape RangeShape, const FVector& Location, float Radius, float Width, float Height, FVector RGB)
{
    // 생성
    if (IsValid(RangeIndicator) == false)
    {
        AActor* AvatarActor = GetPawn();
        if (!AvatarActor)
            return;
        
        RangeIndicator = GetWorld()->SpawnActor<AAbilityRangeIndicator>(RangeIndicatorClass);
        RangeIndicator->InitializeIndicatorParams(
            AvatarActor, true, RangeShape, Location,
            Radius, Width, Height, 0.f,
            RGB);
    }
}

void AAuraPlayerController::HideRangeIndicator()
{
    if (IsValid(RangeIndicator))
    {
        AActor* AvatarActor = GetPawn();
        if (!AvatarActor)
            return;
        
        RangeIndicator->Destroy();
    }
}

void AAuraPlayerController::SetTargetingStatus(ETargetingStatus InStatus)
{
    TargetingStatus = InStatus;
}

void AAuraPlayerController::HandleCardSelectionInitialized()
{
    if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetHUD()))
    {
        if (UMVVM_CardSelection* CardSelectionViewModel = AuraHUD->GetCardSelectionViewModel())
        {
            // 리롤 버튼
            if (!CardSelectionViewModel->OnRerollSelectedDelegate.IsAlreadyBound(this, &AAuraPlayerController::HandleAbilityCardRerollSelected))
                CardSelectionViewModel->OnRerollSelectedDelegate.AddDynamic(this, &AAuraPlayerController::HandleAbilityCardRerollSelected);
            
            // 카드 선택
            if (!CardSelectionViewModel->OnUpgradeSelectedOnCardDelegate.IsAlreadyBound(this, &AAuraPlayerController::HandleAbilityCardSelected))
                CardSelectionViewModel->OnUpgradeSelectedOnCardDelegate.AddDynamic(this, &AAuraPlayerController::HandleAbilityCardSelected);
            
            // for (int32 i = 0; i < CardSelectionViewModel->GetNumCards(); ++i)
            // {
            //     if (UMVVM_AbilityCard* CardViewModel = CardSelectionViewModel->GetCardViewModelByIndex(i))
            //     {
            //         if (!CardViewModel->OnUpgradeSelectedDelegate.IsAlreadyBound(this, &AAuraPlayerController::HandleAbilityCardSelected))
            //             CardViewModel->OnUpgradeSelectedDelegate.AddDynamic(this, &AAuraPlayerController::HandleAbilityCardSelected);
            //     }
            // }
            
            // 게임 일시정지 요청
            Server_RequestPauseGame(this);
        }
    }
}

void AAuraPlayerController::HandleAbilityCardSelected(FGameplayTag SelectedUpgradeTag)
{
    Server_SelectUpgrade(SelectedUpgradeTag);
    
    Server_RequestUnPauseGame(this);
    
    // 델리게이트 언바인드
    if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetHUD()))
    {
        if (UMVVM_CardSelection* CardSelectionViewModel = AuraHUD->GetCardSelectionViewModel())
        {
            // for (int32 i = 0; i < CardSelectionViewModel->GetNumCards(); ++i)
            // {
            //     if (UMVVM_AbilityCard* CardViewModel = CardSelectionViewModel->GetCardViewModelByIndex(i))
            //     {
            //         CardViewModel->OnUpgradeSelectedDelegate.Clear();
            //     }
            // }
            CardSelectionViewModel->OnUpgradeSelectedOnCardDelegate.Clear();
        }
    }
    
    // 카드 선택 UI 닫기
    if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetHUD()))
    {
        if (AuraHUD->CardSelectionWidget)
        {
            AuraHUD->CardSelectionWidget->RemoveFromParent();
            AuraHUD->CardSelectionWidget = nullptr;
        }
    }
}

void AAuraPlayerController::HandleAbilityInfoCardSelected(TArray<FAuraAbilityUpgradeInfo>& SelectedUpgradeInfo)
{
    Server_SelectUpgrade(SelectedUpgradeInfo[0].UpgradeEffectTag);

    Server_RequestUnPauseGame(this);
    
    // 카드 선택 UI 닫기
    if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetHUD()))
    {
        AuraHUD->CardSelectionWidget->RemoveFromParent();
        AuraHUD->CardSelectionWidget = nullptr;
    }
}

void AAuraPlayerController::HandleAbilityCardRerollSelected()
{
    if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetHUD()))
    {
        if (UMVVM_CardSelection* CardSelectionViewModel = AuraHUD->GetCardSelectionViewModel())
        {
            // for (int32 i = 0; i < CardSelectionViewModel->GetNumCards(); ++i)
            // {
            //     if (UMVVM_AbilityCard* CardViewModel = CardSelectionViewModel->GetCardViewModelByIndex(i))
            //     {
            //         CardViewModel->OnUpgradeSelectedDelegate.Clear();
            //     }
            // }
            CardSelectionViewModel->OnRerollSelectedDelegate.Clear();
        }

        // 위젯 제거
        if (AuraHUD->CardSelectionWidget)
        {
            AuraHUD->CardSelectionWidget->RemoveFromParent();
            AuraHUD->CardSelectionWidget = nullptr;
        }
    }
    
    Server_CreateCardSelection(GetPawn());
}

void AAuraPlayerController::HandleInventoryUIInit()
{
    if (AAuraPlayerState* MyPS = GetPlayerState<AAuraPlayerState>())
    {
        if (AAuraHUD* AuraHUD = GetHUD<AAuraHUD>())
        {
            FWidgetControllerParams WCParams;
            WCParams.PlayerController = this;
            WCParams.AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(this->GetPawn());
            WCParams.PlayerState = MyPS;
            WCParams.AttributeSet = MyPS->GetAttributeSet();
					
            // 뷰 모델 생성
            if (auto ViewModel = AuraHUD->GetInventoryViewModel(WCParams))
            {
                ViewModel->InitializeSlots();
            }
        }
    }
}

void AAuraPlayerController::SetAutoRunDestination(const FVector& Destination, const TArray<FVector>& PathPoints)
{
    CachedDestination = Destination;
    bAutoRunning = true;
    
    Spline->ClearSplinePoints();
    for (const FVector& Point : PathPoints)
    {
        Spline->AddSplinePoint(Point, ESplineCoordinateSpace::World);
    }
}

// void AAuraPlayerController::Server_StopAutoRun_Implementation()
// {
//     bAutoRunning = false;
//     if (GetPawn())
//         CachedDestination = GetPawn()->GetActorLocation();
// }


void AAuraPlayerController::Client_RemoveCardSelection_Implementation()
{
    if (auto AuraHUD = GetHUD<AAuraHUD>())
    {
        if (UMVVM_CardSelection* CardSelectionViewModel = AuraHUD->GetCardSelectionViewModel())
        {
            CardSelectionViewModel->OnCloseSelectedDelegate.Broadcast();
        }
    }
}

void AAuraPlayerController::SaveCharacterProgress_Implementation(FName PlayerStartTag)
{
    if (!GetPawn() || GetPawn()->IsPendingKillPending())
        return;
    
    if (GetPawn()->Implements<UPlayerInterface>())
    {
        IPlayerInterface::Execute_SaveProgress( GetPawn(), PlayerStartTag);
    }
}

void AAuraPlayerController::Server_RequestUnPauseGame_Implementation(AAuraPlayerController* RequestedPC)
{
    FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(TEXT("Message.GamePausedBy"));
    FText AppendText = FText::FromString(RequestedPC->PlayerState->GetPlayerName());
    
    // 게임 배속을 원래대로 변경
    UGameplayStatics::SetGlobalTimeDilation(this, 1.f);
    
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AAuraPlayerController* PC = Cast<AAuraPlayerController>(It->Get());
        if (PC && PC != this)
        {
            UAuraAbilitySystemLibrary::RemoveMessageTagEffectToSelf(PC->GetASC(), MessageTag);
        }
    }
}

void AAuraPlayerController::Server_RequestPauseGame_Implementation(AAuraPlayerController* RequestedPC)
{
    FGameplayTag MessageTag = FGameplayTag::RequestGameplayTag(TEXT("Message.GamePausedBy"));
    FText AppendText = FText::FromString(RequestedPC->PlayerState->GetPlayerName());
    
    for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AAuraPlayerController* PC = Cast<AAuraPlayerController>(It->Get());
        if (PC && PC != this)
        {
            UAuraAbilitySystemLibrary::AddMessageToActor(PC->GetPawn(), MessageTag, AppendText);
        }
    }
    
    // 게임 배속을 느리게 지정
    UGameplayStatics::SetGlobalTimeDilation(this, 0.05f);
}

void AAuraPlayerController::Server_TryRemoveItem_Implementation(int32 SlotIndex)
{
    // 서버가 아니면 리턴
    if (!HasAuthority())
        return;
    
    UInventoryComponent* Inventory = IPlayerInterface::Execute_GetInventoryComponent(GetPawn());
	if (!Inventory)
	    return;
    
    auto& InventorySlots = Inventory->GetSlots();
    
    if (!InventorySlots.IsValidIndex(SlotIndex))
        return;
	
    const FInventorySlot& TargetSlot = InventorySlots[SlotIndex];
    const FItemData ItemDataToDrop = TargetSlot.ItemData;
    const FIntPoint StartPoint = TargetSlot.StartPoint;
    const FIntPoint ItemSize = TargetSlot.ItemSize;
			
    Inventory->ClearItemSpace_Internal(StartPoint, ItemSize);
    Inventory->OnItemRemoved.Broadcast(ItemDataToDrop);
    
    // 월드에 아이템 액터 스폰
    if (auto AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
    {
        if (auto AuraCharacter = Cast<AAuraCharacter>(GetPawn()))
            AuraGM->SpawnDropItemToActorLocation(AuraCharacter, ItemDataToDrop);
    }
}

void AAuraPlayerController::Server_TryPickUpItem_Implementation(AAuraDropItem* DropItem, AAuraPlayerController* OwnerPC)
{
    if (!DropItem || DropItem->IsPendingKillPending())
        return;
    
    if (DropItem->bIsPickedUp)
        return;
    
    DropItem->bIsPickedUp = true;
        
    FVector AuraLocation = OwnerPC->GetPawn()->GetActorLocation();
    FVector ItemLocation = DropItem->GetActorLocation();
    
    float Distance = FVector::Dist(AuraLocation, ItemLocation);
    if (Distance <= 150.f)
    {
        const FItemData& DropItemData = DropItem->DropItemData;
        if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
        {
            if (AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(GetPawn()))
            {
                // 서버 RPC 함수가 작동 중일 때, 중복 습득을 못하게 막음
                if (AuraGM->GiveItemToCharacter(AuraCharacter, DropItemData, DropItemData.ItemCounts))
                {
                    DropItem->Destroy();
                }
                else
                {
                    // 슬롯 부족, 또는 아이템 데이터 검색 실패
                    DropItem->bIsPickedUp = false;
                    DropItem->SetActorHiddenInGame(false);
                    DropItem->SetActorEnableCollision(true);
                    UAuraAbilitySystemLibrary::AddMessageToActor(AuraCharacter, FGameplayTag::RequestGameplayTag("Message.InventoryFull"), FText());
                }
            }
        }
    }
    else
    {
        DropItem->bIsPickedUp = false;
    }
    OwnerPC->Client_OnPickUpFinished();
}

void AAuraPlayerController::Server_StartSpectating_Implementation()
{
    if (!HasAuthority())
        return;
    
    if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
    {
        AActor* SpectatingPawn = nullptr;
        for (auto It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (!PC)
                continue;
            
            AAuraPlayerController* SpectatingPlayer = Cast<AAuraPlayerController>(PC);
            if (!SpectatingPlayer)
                continue;
            
            SpectatingPawn = SpectatingPlayer->GetPawn();
            if (SpectatingPawn && SpectatingPawn != GetPawn())
            {
                break;
            }
        }
        
        if (SpectatingPawn)
            SetViewTargetWithBlend(SpectatingPawn, 0.5f);
    }
}

void AAuraPlayerController::Server_ReviveFromPlayerStart_Implementation()
{
    if (!HasAuthority())
        return;
    
    if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        // 서버에서 부활
        AuraGM->PlayerRespawn(this);
    }
}

void AAuraPlayerController::Server_CharacterDebugInvincible_Implementation(bool bInvincible)
{
    if (!HasAuthority())
        return;

    if (AAuraCharacterBase* AuraCharacterBase = GetPawn<AAuraCharacterBase>())
    {
        if (AuraCharacterBase->Implements<UCombatInterface>())
        {
            ICombatInterface::Execute_SetCharacterDebugInvincible(AuraCharacterBase, bInvincible);
        }
    }
}

void AAuraPlayerController::Server_CharacterInvincible_Implementation(bool bInvincible)
{
    if (!HasAuthority())
        return;

    if (AAuraCharacterBase* AuraCharacterBase = GetPawn<AAuraCharacterBase>())
    {
        if (AuraCharacterBase->Implements<UCombatInterface>())
        {
            ICombatInterface::Execute_SetCharacterInvincible(AuraCharacterBase, bInvincible);
        }
    }
}

void AAuraPlayerController::Server_CharacterInfiniteMana_Implementation(bool bInfiniteMana)
{
    if (!HasAuthority())
        return;

    if (AAuraCharacterBase* AuraCharacterBase = GetPawn<AAuraCharacterBase>())
    {
        if (AuraCharacterBase->Implements<UCombatInterface>())
        {
            ICombatInterface::Execute_SetCharacterInfiniteMana(AuraCharacterBase, bInfiniteMana);
        }
    }
}

void AAuraPlayerController::Server_AddAbilityToPlayerByGameplayTag_Implementation(const FGameplayTag& Tag)
{
    if (!HasAuthority())
        return;

    if (auto* AuraASC = GetASC())
    {
        AuraASC->AddCharacterAbilityByTag(Tag);
    }
}

void AAuraPlayerController::Server_CreateCardSelection_Implementation(AActor* InteractedActor)
{
    if (!HasAuthority())
        return;
    
    // 게임 모드에게 카드 뽑기를 요청
    if (AAuraGameModeBase* GameMode = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        if (APawn* InteractedPawn = Cast<APawn>(InteractedActor))
        {
            if (APlayerController* PC = Cast<APlayerController>(InteractedPawn->GetController()))
            {
                // GameMode의 카드 뽑기 로직 호출
                GameMode->HandleInitializeCards(PC);
            }
        }
    }
}

void AAuraPlayerController::Server_RemoveCardSelection_Implementation(AActor* InteractedActor)
{
    if (!HasAuthority())
        return;
    
    Client_RemoveCardSelection();
}

void AAuraPlayerController::Server_RemoveUpgrade_Implementation(FGameplayTag RemoveTag)
{
    if (!HasAuthority())
        return;
    
    AActor* AvatarActor = GetPawn();
    if (AvatarActor == nullptr)
        return;

    // 업그레이드 태그 제거
    if (AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>())
    {
        // 플레이어 상태의 태그 컨테이너에서 해당 태그 제거
        AuraPlayerState->Server_RemoveAbilityUpgradeTag(RemoveTag);
    }
}

void AAuraPlayerController::Server_SelectUpgrade_Implementation(FGameplayTag SelectedUpgradeTag)
{
    if (!HasAuthority())
        return;
    
    AActor* AvatarActor = GetPawn();
    if (AvatarActor == nullptr)
        return;
    
    // 빈 태그면 리턴
    if (SelectedUpgradeTag.MatchesTag(FGameplayTag::EmptyTag))
    {
        // 자동 저장
        if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
        {
            AuraGM->GameAutoSave();
        }
        return;
    }

    // 업그레이드 태그 적용
    if (AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>())
    {
        // 플레이어 상태의 태그 컨테이너 내에 저장
        AuraPlayerState->Server_AddAbilityUpgradeTag(SelectedUpgradeTag);
        
        // 자동 저장
        if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
        {
            AuraGM->GameAutoSave();
        }
    }
}

void AAuraPlayerController::Client_ShowGameOverWidget_Implementation()
{
    UAuraAbilitySystemLibrary::GetGameOverWidgetController(this)->HandleOnDeath(GetPawn());
}

void AAuraPlayerController::Client_CreateMessageWidget_Implementation(const FGameplayTag& MessageTag, const FText& AppendText, UTexture2D* Icon)
{
    if (auto* GI = Cast<UAuraGameInstance>(this->GetGameInstance()))
    {
        if (FUIWidgetRow* FoundRow = GI->MessageTable->FindRow<FUIWidgetRow>(MessageTag.GetTagName(), "Found Message", false))
        {
            TSubclassOf<UAuraUserWidget> MessageWidgetClass = FoundRow->MessageWidget;
            FText FoundMessage = FoundRow->Message;
            FText FinalMessage = FText();
            if (!FoundMessage.IsEmpty())
            {
                FFormatOrderedArguments Args;
                Args.Add(AppendText);
                FinalMessage = FText::Format(FoundMessage, Args);
            }
            if (auto AuraHUD = GetHUD<AAuraHUD>())
            {
                AuraHUD->CreateMessageWidget(MessageWidgetClass, FinalMessage, Icon);
            }
        }
    }
}

void AAuraPlayerController::Server_ApplyWaitForExecuteTag_Implementation()
{
    if (auto AuraGI = GetGameInstance<UAuraGameInstance>())
    {
        auto EffectContext = GetASC()->MakeEffectContext();
        auto Spec = GetASC()->MakeOutgoingSpec(AuraGI->WaitForExecute, 1.0f, EffectContext);
            
        GetASC()->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}

void AAuraPlayerController::Client_OnPickUpFinished_Implementation()
{
    bIsPickingUpItem = false;
}

void AAuraPlayerController::Move(const FInputActionValue& InputActionValue)
{
    const FVector2D InputAxisVector = InputActionValue.Get<FVector2D>();
    const FRotator Rotation = GetControlRotation();
    const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

    // 회전자의 X축 방향(전방)과 Y축 방향(오른쪽 방향) 단위 벡터를 찾아옴
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    if (APawn* ControlledPawn = GetPawn<APawn>())
    {
        ControlledPawn->AddMovementInput(ForwardDirection, InputAxisVector.Y);
        ControlledPawn->AddMovementInput(RightDirection, InputAxisVector.X);
    }
    
    UAbilitySystemComponent* ASC = GetASC();
    if (ASC && ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
    {
        if (APawn* ControlledPawn = GetPawn())
        {
            if (ACharacter* AuraCharacter = Cast<ACharacter>(ControlledPawn))
                AuraCharacter->GetCharacterMovement()->StopMovementImmediately();
        }
        return;
    }
    
    SetAutoRunning(false);
}

void AAuraPlayerController::Zoom(const struct FInputActionValue& InputActionValue)
{
    // 입력 상태 태그 확인
    if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
    {
        return;
    }

    const float InputAxisValue = InputActionValue.Get<float>();

    if (AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(GetPawn<APawn>()))
    {
        if (auto* SpringArm = AuraCharacter->GetSpringArmComponent().Get())
        {
            // 카메라 길이
            float NewArmLength = SpringArm->TargetArmLength - (100.f * InputAxisValue);
            float ClampingLength = FMath::Clamp(NewArmLength, 200, 1000);

            SpringArm->TargetArmLength = ClampingLength;

            // 카메라 회전
            float Pitch = SpringArm->GetRelativeRotation().Pitch;
            float NewPitch = Pitch + (5.f * InputAxisValue);
            float ClampedPitch = FMath::Clamp(NewPitch, -45.f, 0.f);
            
            FRotator Rotator = SpringArm->GetRelativeRotation();
            Rotator.Pitch = ClampedPitch;

            // 휠을 올리면 점점 0으로 가야함(+ 방향)
            SpringArm->SetRelativeRotation(Rotator);

            // FadeActor 감지 박스
            if (auto* Box = AuraCharacter->GetBoxComponent().Get())
            {
                // 확대하면 X축 상대 스케일이 점점 줄어들어야함
                FVector BoxLocation(SpringArm->TargetArmLength / 2, 0.f, 0.f);
                Box->SetRelativeLocation(BoxLocation);
                
                FVector CurrentBoxScale = Box->GetRelativeScale3D();
                float NewBoxScale_X = CurrentBoxScale.X - 0.8375f * InputAxisValue;
                float ClampBoxScale_X = FMath::Clamp(NewBoxScale_X, 1.675f, 10.05f);

                CurrentBoxScale.X = ClampBoxScale_X;
                
                Box->SetRelativeScale3D(CurrentBoxScale);
            }
        }
    }
}

void AAuraPlayerController::ActivateDebugMode(const struct FInputActionValue& InputActionValue)
{
    if (UOverlayWidgetController* OverlayWC = UAuraAbilitySystemLibrary::GetOverlayWidgetController(this))
    {
        if (bDebugModeActivated)
        {
            bDebugModeActivated = false;
            OverlayWC->OnDebugModeActivated.Broadcast(false);
            return;
        }
        
        bDebugModeActivated = true;
        OverlayWC->OnDebugModeActivated.Broadcast(true);
    }
}

void AAuraPlayerController::ShowAttributeMenu()
{
    UAuraAbilitySystemLibrary::GetOverlayWidgetController(this)->OnAttributeMenuKeyPressed.Broadcast();
}

void AAuraPlayerController::ShowSpellMenu()
{
    UAuraAbilitySystemLibrary::GetOverlayWidgetController(this)->OnSpellMenuKeyPressed.Broadcast();
}

void AAuraPlayerController::ShowESCMenu()
{
    UAuraAbilitySystemLibrary::GetOverlayWidgetController(this)->OnESCMenuKeyPressed.Broadcast();
}

void AAuraPlayerController::ShowInventoryMenu()
{
    UAuraAbilitySystemLibrary::GetOverlayWidgetController(this)->OnInventoryMenuKeyPressed.Broadcast();
}

void AAuraPlayerController::ShowItemTitle(const FInputActionValue& Value)
{
    if (auto AuraState = GetWorld()->GetGameState<AAuraGameStateBase>())
    {
        auto& Items = AuraState->GetDroppedItemsArray();
        const bool InputValue = Value.Get<bool>();
        if (InputValue)
        {
            for (auto& Item : Items)
            {
                Item->SetTitleWidgetVisibility(true);
            }
        }
        else
        {
            for (auto& Item : Items)
            {
                Item->SetTitleWidgetVisibility(false);
            }
        }
    }
}

void AAuraPlayerController::CursorTrace()
{
    if (GetWorld() && GetWorld()->bIsTearingDown) 
        return;
    
    // 입력 상태 태그 확인
    if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_CursorTrace))
    {
        if (LastActor)
            UnHighlightActor(LastActor);
        
        if (ThisActor)
            UnHighlightActor(ThisActor);
        
        LastActor = nullptr;
        ThisActor = nullptr;
        return;
    }

    // 트레이스 채널, 단순 충돌 확인, 반환되는 FHitResult 구조체의 주소
    ECollisionChannel TraceChannel = ECollisionChannel::ECC_Visibility;
    
    // 매직 서클이 표시중이면 트레이스 채널 변경
    if (IsValid(MagicCircle))
        TraceChannel = ECC_GroundCheck;
    
    LastActor = ThisActor;

    GetHitResultUnderCursor(TraceChannel, false, CursorHit);

    // 진짜 유효한 적일때만 유지
    if (CursorHit.bBlockingHit && IsValid(CursorHit.GetActor()) && CursorHit.GetActor()->Implements<UHighlightInterface>())
    {
        ThisActor = CursorHit.GetActor();
    }
    else
    {
        ThisActor = nullptr; 
    }
    
    if (LastActor == nullptr)
    {
        if (ThisActor != nullptr)
        {
            // 허공에 있다가 처음으로 적에게 마우스를 올림
            HighlightActor(ThisActor);
        }
    }
    else // LastActor가 존재함 (이전에 적을 가리키고 있었음)
    {
        if (ThisActor == nullptr)
        {
            UnHighlightActor(LastActor);
        }
        else 
        {
            if (LastActor != ThisActor)
            {
                UnHighlightActor(LastActor);
                HighlightActor(ThisActor);
            }
        }
    }
}

void AAuraPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
    // 입력 상태 태그 확인
    UAuraAbilitySystemComponent* ASC = GetASC();
    if (!ASC)
        return;
    
    if (AAuraCharacterBase* Aura = Cast<AAuraCharacterBase>(GetPawn()))
    {
        if (ICombatInterface::Execute_IsDead(Aura))
            return;
    }
    
    if (ASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
    {
        StopAutoRun();
        return;
    }

    if (InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
    {
        if (IsValid(ThisActor))
        {
            // ThisActor에 따라 타겟팅 상태 전환
            if (ThisActor->Implements<UCombatInterface>())
            {
                bool IsThisActorDead = ICombatInterface::Execute_IsDead(ThisActor);
                if (IsThisActorDead)
                {
                    TargetingStatus = ETargetingStatus::None;
                    UnHighlightActor(ThisActor);
                    return;
                }
            }
            
            if (ThisActor->Implements<UEnemyInterface>())
            {
                TargetingStatus = ETargetingStatus::TargetingEnemy;
                TargetItem = nullptr;
            }
            else if (ThisActor->Implements<UItemInterface>())
            {
                TargetingStatus = ETargetingStatus::TargetingItem;
                TargetItem = ThisActor;
            }
            else
            {
                TargetingStatus = ETargetingStatus::TargetingNonEnemy;
                TargetItem = nullptr;
                return;
            }
        }
        else
        {
            TargetItem = nullptr;
            TargetingStatus = ETargetingStatus::None;
        }
    }
    
    if (TargetingStatus == ETargetingStatus::TargetingItem)
    {
        TargetItem = ThisActor;
        return;
    }

    if (ASC)
        ASC->AbilityInputTagPressed(InputTag);
}

void AAuraPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
    UAuraAbilitySystemComponent* ASC = GetASC();
    if (!ASC)
        return;
    
    if (GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
    {
        StopAutoRun();
        return;
    }
    
    if (AAuraCharacterBase* Aura = Cast<AAuraCharacterBase>(GetPawn()))
    {
        if (ICombatInterface::Execute_IsDead(Aura))
            return;
    }
    
    // 더 이상 왼쪽 클릭 태그가 아닐 경우
    if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
    {
        if (ASC)
            ASC->AbilityInputTagReleased(InputTag);

        return;
    }

    // 타겟이 없고 쉬프트키가 눌리지 않았다면 + 마우스로 이동일 때
    if (bAllowMoveToMouse)
    {
        if (TargetingStatus != ETargetingStatus::TargetingEnemy && !bShiftKeyDown)
        {
            // 경계값보다 짧게 눌렀으면 목적지로 길 찾기 시작
            const APawn* ControlledPawn = GetPawn();
            if (FollowTime <= ShortPressThreshold && ControlledPawn)
            {
                if (IsValid(ThisActor) && ThisActor->Implements<UHighlightInterface>())
                {
                    IHighlightInterface::Execute_SetMoveToLocation(ThisActor, CachedDestination);    
                }
                else if (GetASC() && !GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
                {
                    UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ClickNiagaraSystem, CachedDestination);
                }
            
                if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, ControlledPawn->GetActorLocation(), CachedDestination))
                {
                    // 각 경로 점을 스플라인에 추가
                    Spline->ClearSplinePoints();
                    for (const FVector& PointLoc : NavPath->PathPoints)
                    {
                        Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
                    }

                    if (NavPath->PathPoints.Num() > 0)
                    {
                        CachedDestination = NavPath->PathPoints[NavPath->PathPoints.Num() - 1];
                        bAutoRunning = true;
                        
                        // 서버로 이동 경로 전달
                        SetAutoRunDestination(CachedDestination, NavPath->PathPoints);
                    }
                }
            }
        }
        else
        {
            if (ASC)
                ASC->AbilityInputTagReleased(InputTag);
        }
    }
    else
    {
        if (TargetingStatus == ETargetingStatus::TargetingItem || TargetingStatus == ETargetingStatus::TargetingNonEnemy)
        {
            // 아이템 또는 상호작용 액터를 클릭했을 때
            // 경계값보다 짧게 눌렀으면 목적지로 길 찾기 시작
            const APawn* ControlledPawn = GetPawn();
            if (FollowTime <= ShortPressThreshold && ControlledPawn)
            {
                if (IsValid(ThisActor) && ThisActor->Implements<UHighlightInterface>())
                {
                    IHighlightInterface::Execute_SetMoveToLocation(ThisActor, CachedDestination);    
                }
                
                if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, ControlledPawn->GetActorLocation(), CachedDestination))
                {
                    // 각 경로 점을 스플라인에 추가
                    Spline->ClearSplinePoints();
                    for (const FVector& PointLoc : NavPath->PathPoints)
                    {
                        Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
                    }

                    if (NavPath->PathPoints.Num() > 0)
                    {
                        CachedDestination = NavPath->PathPoints[NavPath->PathPoints.Num() - 1];
                        bAutoRunning = true;
                    }
                }
            }
        }
        else if (TargetingStatus == ETargetingStatus::TargetingEnemy || bShiftKeyDown)
        {  
            if (ASC)
                ASC->AbilityInputTagReleased(InputTag);
        }
    }
    FollowTime = 0.f;
    TargetingStatus = ETargetingStatus::None;
}


void AAuraPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
    UAuraAbilitySystemComponent* ASC = GetASC();
    if (!ASC)
        return;
        
    if (AAuraCharacterBase* Aura = Cast<AAuraCharacterBase>(GetPawn()))
    {
        if (ICombatInterface::Execute_IsDead(Aura))
            return;
    }
    
    if (GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputHeld))
    {
        StopAutoRun();
        return;
    }

    // 더 이상 왼쪽 클릭 태그가 아닐 경우
    if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
    {
        if (ASC)
            ASC->AbilityInputTagHeld(InputTag);

        return;
    }

    // 타겟
    if (TargetingStatus == ETargetingStatus::TargetingEnemy || bShiftKeyDown)
    {
        if (ASC)
            ASC->AbilityInputTagHeld(InputTag);
    }
    else // 이동
    {
        if (bAllowMoveToMouse)
        {
            bAutoRunning = false;
        
            FollowTime += GetWorld()->GetDeltaSeconds();

            if (CursorHit.bBlockingHit)
            {
                // Hit.Location도 사용 가능
                CachedDestination = CursorHit.ImpactPoint;
            }

            if (APawn* ControlledPawn = GetPawn())
            {
                // 방향 벡터 구하기
                const FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
                ControlledPawn->AddMovementInput(WorldDirection);
            }
        }
    }
}

UAuraAbilitySystemComponent* AAuraPlayerController::GetASC()
{
    if (AuraAbilitySystemComponent == nullptr)
    {
        AuraAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
    }

    return AuraAbilitySystemComponent;    
}

