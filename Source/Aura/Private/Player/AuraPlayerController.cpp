// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerController.h"
#include "Interaction/EnemyInterface.h"
#include "EnhancedInputSubsystems.h"
#include "Input/AuraInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
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
#include "Actor/MagicCircle.h"
#include "Components/DecalComponent.h"
#include "Aura/Aura.h"
#include "Character/AuraBossMonster.h"
#include "Game/AuraGameModeBase.h"
#include "Interaction/CombatInterface.h"
#include "Interaction/HighlightInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"
#include "UI/ViewModel/MVVM_AbilityCard.h"
#include "UI/ViewModel/MVVM_CardSelection.h"
#include "UI/Widget/LoadScreenWidget.h"

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
    OnCardSelectedDelegate.AddUObject(this ,&AAuraPlayerController::HandleCardSelectionInitialized);
}

void AAuraPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // 게임 모드의 보스 배열에 접근하여 몽타주 이벤트에 바인딩
    if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(GetWorld()->GetAuthGameMode()))
    {
        auto BossArray = AuraGameMode->GetBossCharactersArray();
        for (const auto Boss : BossArray)
        {
            Boss.Get()->OnBossEventStart.AddDynamic(this, &AAuraPlayerController::OnBossEventStart);
            Boss.Get()->OnBossEventEnd.AddDynamic(this, &AAuraPlayerController::OnBossEventEnd);
            Boss.Get()->OnDeath.AddDynamic(this, &AAuraPlayerController::OnBossDead);
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
    
    // 어빌리티와 입력 액션 바인딩
    AuraInputComponent->BindAbiltyActions(InputConfig, this, &ThisClass::AbilityInputTagPressed, &ThisClass::AbilityInputTagReleased, &ThisClass::AbilityInputTagHeld);
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    // 커서 추적
    CursorTrace();

    // 클릭으로 이동
    AutoRun();

    // 범위 지정 데칼
    UpdateMagicCircleLocation();

    //
    UpdateRangeIndicatorRotation();
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

void AAuraPlayerController::StopAutoRun()
{
    bAutoRunning = false;
    CachedDestination = FVector::ZeroVector;
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

void AAuraPlayerController::UpdateMagicCircleLocation()
{
    if (IsValid(MagicCircle))
    {
        // 빈 공간에서 데칼 숨기기
        if (CursorHit.bBlockingHit)
        {
            MagicCircle->SetActorHiddenInGame(false);
            MagicCircle->SetActorLocation(CursorHit.ImpactPoint);
            MagicCircle->KeepMagicCircleInRange();
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
        MagicCircle->CircleRange = InRange;
        MagicCircle->Radius = InRadius;
        MagicCircle->SetOwner(AvatarActor);
        MagicCircle->CircleInitialized.Broadcast(AvatarActor);
        
        if (DecalMaterial)
        {
            MagicCircle->MagicCircleDecal->SetMaterial(0, DecalMaterial);
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

    // 카드 선택 UI 닫기
    if (AAuraHUD* AuraHUD = Cast<AAuraHUD>(GetHUD()))
    {
        AuraHUD->CardSelectionWidget->RemoveFromParent();
        AuraHUD->CardSelectionWidget = nullptr;
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
    Server_CreateCardSelection(GetPawn());
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
    }
}

void AAuraPlayerController::Move(const FInputActionValue& InputActionValue)
{
    // 입력 상태 태그 확인
    if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
    {
        return;
    }

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
    if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputPressed))
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
            
            TargetingStatus = ThisActor->Implements<UEnemyInterface>() ? ETargetingStatus::TargetingEnemy : ETargetingStatus::TargetingNonEnemy;
        }
        else
        {
            TargetingStatus = ETargetingStatus::None;
        }
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
    if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputReleased))
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
    if (GetASC() && GetASC()->HasMatchingGameplayTag(FAuraGameplayTags::Get().Player_Block_InputHeld))
    {
        bAutoRunning = false;
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

