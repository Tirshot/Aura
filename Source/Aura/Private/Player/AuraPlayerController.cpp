// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerController.h"
#include "Interaction/EnemyInterface.h"
#include "EnhancedInputSubsystems.h"
#include "Input/AuraInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SplineComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "EngineUtils.h"
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
#include "Game/AuraGameModeBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/HighlightInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"
#include "UI/ViewModel/MVVM_AbilityCard.h"
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
    OnCardSelectedDelegate.AddUObject(this, &AAuraPlayerController::HandleCardSelectionInitialized);
    OnReviveTimerEnd.AddDynamic(this, &AAuraPlayerController::Server_ReviveFromPlayerStart);
    
    const FString CurrentLevelName = GetWorld()->GetMapName();

    // 튜토리얼 레벨에서만 위젯 컨트롤러 생성
    if (CurrentLevelName.Contains(TEXT("Tutorial")))
    {
        // 튜토리얼 뷰 모델 생성
        TutorialDialogueViewModel = NewObject<UMVVM_TutorialDialogue>(this);
        TutorialDialogueViewModel->BlueprintInitialize();
        ShowTutorialUI(true);
    }
}

void AAuraPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    OnBossMonsterAdded.AddUObject(this, &AAuraPlayerController::BossMonsterBind);

    if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
    {
        auto BossArray = AuraGM->GetBossCharactersArray();
        if (BossArray.Num() > 0)
        {
            OnBossMonsterAdded.Broadcast();
        }
    }
}

void AAuraPlayerController::AcknowledgePossession(APawn* P)
{
    Super::AcknowledgePossession(P);
    
    // TODO::클라이언트 또한 보스 몬스터 이벤트에 바인딩 해야함 
    
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
    
    if (GetASC())
    {
        if (GetASC()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead"))))
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
    
    // 아이템 클릭 후 근접하면 획득
    if (TargetItem)
    {
        if (AAuraDropItem* DropItem = Cast<AAuraDropItem>(TargetItem))
        {
            FVector AuraLocation = GetPawn()->GetActorLocation();
            FVector ItemLocation = DropItem->GetActorLocation();
            
            float Distance = FVector::Dist(AuraLocation, ItemLocation);
            if (Distance <= 130.f)
            {
                DropItem->Server_AddItemToCharacter(GetPawn());
                TargetItem = nullptr;
            }
        }
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
        }
    }
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
    CachedDestination = FVector::ZeroVector;
}

void AAuraPlayerController::BossMonsterBind()
{
    // 게임 모드의 보스 배열에 접근하여 몽타주 이벤트에 바인딩
    if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        auto BossArray = AuraGameMode->GetBossCharactersArray();
        for (const auto Boss : BossArray)
        {
            if (!Boss.Get()->OnBossEventStart.IsAlreadyBound(this, &AAuraPlayerController::OnBossEventStart))
                Boss.Get()->OnBossEventStart.AddDynamic(this, &AAuraPlayerController::OnBossEventStart);
            
            if (!Boss.Get()->OnBossEventEnd.IsAlreadyBound(this, &AAuraPlayerController::OnBossEventEnd))
                Boss.Get()->OnBossEventEnd.AddDynamic(this, &AAuraPlayerController::OnBossEventEnd);
            
            if (!Boss.Get()->OnDeath.IsAlreadyBound(this, &AAuraPlayerController::OnBossDead))
                Boss.Get()->OnDeath.AddDynamic(this, &AAuraPlayerController::OnBossDead);
        }
    }
}

void AAuraPlayerController::OnBossEventStart(AActor* BossActor)
{
    // 오버레이 감추기, 카메라 전환
    if (AAuraHUD* AuraHUD = GetHUD<AAuraHUD>())
    {
        AuraHUD->HideOverlay();
    }

    // 플레이어 입력 방지
    SetPlayerInputEnable(false);
    
    ChangeCameraToBossActor(BossActor, 0.25f, 3.f);
}

void AAuraPlayerController::OnBossEventEnd(AActor* BossActor)
{
    // 블렌드 종료 후 오버레이 보이기, 카메라 전환
    if (AAuraHUD* AuraHUD = GetHUD<AAuraHUD>())
    {
        AuraHUD->ShowOverlay();
    }

    // 플레이어 입력 활성화
    SetPlayerInputEnable(true);

    ChangeCameraToOwn(0.5f);
}

void AAuraPlayerController::OnBossDead(AActor* BossActor)
{
    // 오버레이 감추기, 카메라 전환
    if (AAuraHUD* AuraHUD = GetHUD<AAuraHUD>())
    {
        AuraHUD->HideOverlay();
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
    SetViewTarget(GetPawn());
}

void AAuraPlayerController::SetPlayerInputEnable(bool bEnable)
{
    if (bEnable)
    {
        // 마우스 커서 활성화
        bShowMouseCursor = true;
        DefaultMouseCursor = EMouseCursor::Default;

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
    if (IsValid(MagicCircle))
    {
        // 빈 공간에서 데칼 숨기기
        if (CursorHit.bBlockingHit)
        {
            MagicCircle->SetActorHiddenInGame(false);
            MagicCircle->SetActorLocation(CursorHit.ImpactPoint);
        }
        else
        {
            MagicCircle->SetActorHiddenInGame(true);
        }
    }
}

void AAuraPlayerController::UpdateRangeIndicatorRotation()
{
    if (IsValid(RangeIndicator))
    {
        // 원형이 아닐 때
        if (RangeIndicator->GetRangeShape() != ERangeShape::ERS_Circle)
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

                float Length = RangeIndicator->GetHeight();

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
    if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
    {
        IHighlightInterface::Execute_HighlightActor(InActor);
    }
}

void AAuraPlayerController::UnHighlightActor(AActor* InActor)
{
    if (IsValid(InActor) && InActor->Implements<UHighlightInterface>())
    {
        IHighlightInterface::Execute_UnHighlightActor(InActor);
    }
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
        
        if (DecalMaterial)
        {
            MagicCircle->SetDecalMaterial(DecalMaterial);
        }
    }
}

void AAuraPlayerController::HideMagicCircle()
{
    if (IsValid(MagicCircle))
    {
        AActor* AvatarActor = GetPawn();
        if (!AvatarActor)
            return;
        
        MagicCircle->RemoveCircle.Broadcast(AvatarActor);
        MagicCircle->Destroy();
    }
}

const FVector AAuraPlayerController::GetMagicCircleLocation()
{
    if (!IsValid(MagicCircle))
        {
            UE_LOG(LogTemp, Warning, TEXT("MagicCircle is InValid!!!"));
            return FVector();
        }
    
    return MagicCircle->GetActorLocation();
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
        RangeIndicator->SetOwner(AvatarActor);
        RangeIndicator->IndicatorInitialized.Broadcast(
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
        
        RangeIndicator->RemoveIndicator.Broadcast(AvatarActor);
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
            if (!CardSelectionViewModel->OnRerollSelectedDelegate.IsBound())
                CardSelectionViewModel->OnRerollSelectedDelegate.AddDynamic(this, &AAuraPlayerController::HandleAbilityCardRerollSelected);
            
            for (int32 i = 0; i < CardSelectionViewModel->GetNumCards(); ++i)
            {
                if (UMVVM_AbilityCard* CardViewModel = CardSelectionViewModel->GetCardViewModelByIndex(i))
                {
                    if (!CardViewModel->OnUpgradeSelectedDelegate.IsBound())
                        CardViewModel->OnUpgradeSelectedDelegate.AddDynamic(this, &AAuraPlayerController::HandleAbilityCardSelected);
                }
            }
        }
    }
}

void AAuraPlayerController::HandleAbilityCardSelected(FGameplayTag SelectedUpgradeTag)
{
    Server_SelectUpgrade(SelectedUpgradeTag);
    
    // 델리게이트 언바인드
    if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetHUD()))
    {
        if (UMVVM_CardSelection* CardSelectionViewModel = AuraHUD->GetCardSelectionViewModel())
        {
            // 업그레이드 선택 완료 알림
            CardSelectionViewModel->OnUpgradeSelectedOnCardDelegate.Broadcast();
            
            for (int32 i = 0; i < CardSelectionViewModel->GetNumCards(); ++i)
            {
                if (UMVVM_AbilityCard* CardViewModel = CardSelectionViewModel->GetCardViewModelByIndex(i))
                {
                    CardViewModel->OnUpgradeSelectedDelegate.Clear();
                }
            }
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
    
    // 자동 저장??
    if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
    {
        AuraGM->Server_SaveWorldState(GetWorld());
    }
}

void AAuraPlayerController::HandleAbilityInfoCardSelected(TArray<FAuraAbilityUpgradeInfo>& SelectedUpgradeInfo)
{
    Server_SelectUpgrade(SelectedUpgradeInfo[0].UpgradeEffectTag);

    // 카드 선택 UI 닫기
    if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetHUD()))
    {
        AuraHUD->CardSelectionWidget->RemoveFromParent();
        AuraHUD->CardSelectionWidget = nullptr;
    }
}

void AAuraPlayerController::HandleAbilityCardRerollSelected()
{
    if (!HasAuthority())
        return;
    
    Server_CreateCardSelection(GetPawn());
}

void AAuraPlayerController::Client_ShowGameOverWidget_Implementation()
{
    UAuraAbilitySystemLibrary::GetGameOverWidgetController(this)->HandleOnDeath(GetPawn());
}

void AAuraPlayerController::Server_StartSpectating_Implementation()
{
    if (!HasAuthority())
        return;
    
    if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
    {
        AActor* SpectatingPawn = nullptr;
        for (TSoftObjectPtr<AAuraPlayerController> SpectatingPlayer : AuraGM->GetPlayersArray())
        {
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

    // 업그레이드 태그 적용
    if (AAuraPlayerState* AuraPlayerState = GetPlayerState<AAuraPlayerState>())
    {
        // 플레이어 상태의 태그 컨테이너 내에 저장
        AuraPlayerState->Server_AddAbilityUpgradeTag(SelectedUpgradeTag);
        
        // 자동 저장
        if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
        {
            AuraGM->Server_GameAutoSave();
        }
    }
}

void AAuraPlayerController::Move(const FInputActionValue& InputActionValue)
{
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
    const bool InputValue = Value.Get<bool>();
    if (InputValue)
    {
        for (TActorIterator<AAuraDropItem> It(GetWorld()); It; ++It)
        {
            It->SetTitleWidgetVisibility(true);
        }
    }
    else
    {
        for (TActorIterator<AAuraDropItem> It(GetWorld()); It; ++It)
        {
            It->SetTitleWidgetVisibility(false);
        }
    }
}

void AAuraPlayerController::CursorTrace()
{
    // 입력 상태 태그 확인
    if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_CursorTrace))
    {
        UnHighlightActor(LastActor);
        UnHighlightActor(ThisActor);
        
        if (IsValid(ThisActor) && ThisActor->Implements<UHighlightInterface>())
        {
            LastActor = nullptr;
            ThisActor = nullptr;
            return;
        }
    }

    // 트레이스 채널, 단순 충돌 확인, 반환되는 FHitResult 구조체의 주소
    ECollisionChannel TraceChannel = IsValid(MagicCircle) ? ECC_ExcludePlayers : ECollisionChannel::ECC_Visibility;

    GetHitResultUnderCursor(TraceChannel, false, CursorHit);

    if (!CursorHit.bBlockingHit)
        return;

    // 데칼 표시 중 리턴
    if (IsValid(MagicCircle))
    {
        LastActor = nullptr;
        ThisActor = nullptr;
        return;
    }

    LastActor = ThisActor;
    // 마우스 커서와 충돌한 액터 꺼내오기
    if (IsValid(CursorHit.GetActor()) && CursorHit.GetActor()->Implements<UHighlightInterface>())
    {
        ThisActor = CursorHit.GetActor();
    }
    else
    {
        ThisActor = nullptr;
    }

    if (LastActor != ThisActor)
    {
        UnHighlightActor(LastActor);
        HighlightActor(ThisActor);
    }
}

void AAuraPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
    // 입력 상태 태그 확인
    if (!GetASC())
        return;
    
    if (GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed)
        || GetASC()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead"))))
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
            }
            else if (ThisActor->Implements<UItemInterface>())
            {
                TargetingStatus = ETargetingStatus::TargetingItem;
                TargetItem = ThisActor;
            }
            else
            {
                TargetingStatus = ETargetingStatus::TargetingNonEnemy;
            }
        }
        else
        {
            TargetingStatus = ETargetingStatus::None;
        }
    }
    
    if (TargetingStatus == ETargetingStatus::TargetingItem)
    {
        TargetItem = ThisActor;
    }

    if (GetASC())
    {
        GetASC()->AbilityInputTagPressed(InputTag);
    }
    
    StopAutoRun();
}

void AAuraPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
    // 입력 상태 태그 확인
    if (!GetASC())
        return;
    
    if (GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed)
        || GetASC()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead"))))
    {
        StopAutoRun();
        return;
    }
    
    // 더 이상 왼쪽 클릭 태그가 아닐 경우
    if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
    {
        if (GetASC())
            GetASC()->AbilityInputTagReleased(InputTag);

        return;
    }

    if (GetASC())
         GetASC()->AbilityInputTagReleased(InputTag);

    // 타겟이 없고 쉬프트키가 눌리지 않았다면
    if (TargetingStatus != ETargetingStatus::TargetingEnemy && !bShiftKeyDown)
    {
        // 경계값보다 짧게 눌렀으면 목적지로 길 찾기 시작
        const APawn* ControlledPawn = GetPawn();
        if (FollowTime <= ShortPressThresold && ControlledPawn)
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
                }
            }
        }
        
        FollowTime = 0.f;
        TargetingStatus = ETargetingStatus::None;
    }
}

void AAuraPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
    // 입력 상태 태그 확인
    if (!GetASC())
        return;
    
    if (GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed)
        || GetASC()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead"))))
    {
        StopAutoRun();
        return;
    }

    // 더 이상 왼쪽 클릭 태그가 아닐 경우
    if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
    {
        if (GetASC())
            GetASC()->AbilityInputTagHeld(InputTag);

        return;
    }

    // 타겟
    if (TargetingStatus == ETargetingStatus::TargetingEnemy || bShiftKeyDown)
    {
        if (GetASC())
            GetASC()->AbilityInputTagHeld(InputTag);
    }
    else // 이동
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

UAuraAbilitySystemComponent* AAuraPlayerController::GetASC()
{
    if (AuraAbilitySystemComponent == nullptr)
    {
        AuraAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
    }

    return AuraAbilitySystemComponent;    
}

