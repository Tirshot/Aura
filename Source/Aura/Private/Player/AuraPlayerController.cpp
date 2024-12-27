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

AAuraPlayerController::AAuraPlayerController()
{
    // 서버에서 발생한 변경 사항을 복제하여 모든 클라이언트로 전송(브로드 캐스팅)
    bReplicates = true;

    // 길 찾기 스플라인
    Spline = CreateDefaultSubobject<USplineComponent>("Spline");
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

    // 커서 추적
    CursorTrace();

    // 클릭으로 이동
    AutoRun();
}

void AAuraPlayerController::ShowDamageNumber_Implementation(float DamageAmount, ACharacter* TargetCharacter, bool bBlockedHit, bool bCriticalHit)
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
        DamageText->SetDamageText(DamageAmount, bBlockedHit, bCriticalHit);
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

}

void AAuraPlayerController::CursorTrace()
{
    // 트레이스 채널, 단순 충돌 확인, 반환되는 FHitResult 구조체의 주소
    GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, CursorHit);
    
    if (!CursorHit.bBlockingHit)
        return;

    LastActor = ThisActor;
    // 마우스 커서와 충돌한 액터 꺼내오기
    ThisActor = CursorHit.GetActor();

    if (LastActor != ThisActor)
    {
        if (LastActor)
            LastActor->UnHighlightActor();

        if (ThisActor)
            ThisActor->HighlightActor();
    }
}

void AAuraPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
    if (InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
    {
        // ThisActor가 nullptr이 아니면 true
        bTargeting = ThisActor ? true : false;
        bAutoRunning = false;
    }
}

void AAuraPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
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
    if (!bTargeting && !bShiftKeyDown)
    {
        // 경계값보다 짧게 눌렀으면 목적지로 길 찾기 시작
        const APawn* ControlledPawn = GetPawn();
        if (FollowTime <= ShortPressThresold && ControlledPawn)
        {
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
        bTargeting = false;
    }
}

void AAuraPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
    // 더 이상 왼쪽 클릭 태그가 아닐 경우
    if (!InputTag.MatchesTagExact(FAuraGameplayTags::Get().InputTag_LMB))
    {
        if (GetASC())
            GetASC()->AbilityInputTagHeld(InputTag);

        return;
    }

    // 타겟
    if (bTargeting || bShiftKeyDown)
    {
        if (GetASC())
            GetASC()->AbilityInputTagHeld(InputTag);
    }
    else // 이동
    {
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

