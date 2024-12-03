// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/AuraPlayerController.h"
#include "Interaction/EnemyInterface.h"
#include "EnhancedInputSubsystems.h"
#include "Input/AuraInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"

AAuraPlayerController::AAuraPlayerController()
{
    // 서버에서 발생한 변경 사항을 복제하여 모든 클라이언트로 전송(브로드 캐스팅)
    bReplicates = true;
}

void AAuraPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

    //
    CursorTrace();
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
    FHitResult CursorHit;
    // 트레이스 채널, 단순 충돌 확인, 반환되는 FHitResult 구조체의 주소
    GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, CursorHit);
    
    if (!CursorHit.bBlockingHit)
        return;

    LastActor = ThisActor;

    // 마우스 커서와 충돌한 액터 꺼내오기
    ThisActor = CursorHit.GetActor();

    /* 커서에서 라인트레이스
    (1) LastActor가 nullptr, ThisActor가 nullptr
        - 적이 아닌 액터
        - 아무일도 일어나지 않음
    (2) LastActor가 nullptr, ThisActor가 유효
        - 적에게 처음으로 마우스를 가져다 댐
        - ThisActor->HighlightActor();
    (3) LastActor가 유효, ThisActor가 nullptr
        - 적을 가리키다가 더 이상 마우스가 위치하지 않음
        - LastActor->UnHighlightActor();
    (4) LastActor와 ThisActor 모두 유효, LastActor != ThisActor
        - 적을 가리키다가 다른 적을 가리키게 됨
        - LastActor->UnHighlightActor();
        - ThisActor->HighlightActor();
    (5) LastActor와 ThisActor 모두 유효, LastActor == ThisActor
        - 이미 Highlight 함수가 발동
        - 아무 일도 일어나지 않음*/

    if (LastActor == nullptr)
    {
        if (ThisActor != nullptr)
        {
            // 2번 케이스
            ThisActor->HighlightActor();
        }
        else
        {
            // 1번 케이스

        }
    }
    else // LastActor가 유효
    {
        if (ThisActor == nullptr)
        {
            // 3번 케이스
            LastActor->UnHighlightActor();
        }
        else
        {
            if (LastActor == ThisActor)
            {
                // 5번 케이스

            }
            else
            {
                // 4번 케이스
                LastActor->UnHighlightActor();
                ThisActor->HighlightActor();
            }
        }
    }
}

void AAuraPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{

}

void AAuraPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
    if (GetASC() == nullptr)
        return;

    GetASC()->AbilityInputTagReleased(InputTag);
}

void AAuraPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
    if (GetASC() == nullptr)
        return;

    GetASC()->AbilityInputTagHeld(InputTag);
}

UAuraAbilitySystemComponent* AAuraPlayerController::GetASC()
{
    if (AuraAbilitySystemComponent == nullptr)
    {
        AuraAbilitySystemComponent = Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
    }

    return AuraAbilitySystemComponent;    
}

