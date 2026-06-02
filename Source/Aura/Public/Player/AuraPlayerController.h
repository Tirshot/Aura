// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Actor/AbilityRangeIndicator.h"
#include "Actor/MagicCircle.h"
#include "AuraPlayerController.generated.h"

class AAuraDropItem;
class UAuraUserWidget;
class UMVVM_TutorialDialogue;
class UGameplayAbility;
class AAbilityRangeIndicator;
struct FAuraAbilityUpgradeInfo;
class IHighlightInterface;
class UInputMappingContext;
class UInputAction;
class UAuraInputConfig;
class UAuraAbilitySystemComponent;
class USplineComponent;
class UDamageTextComponent;
class UNiagaraSystem;
class AMagicCircle;

enum class ETargetingStatus : uint8
{
	TargetingEnemy,
	TargetingNonEnemy,
	TargetingItem,
	None
};

DECLARE_MULTICAST_DELEGATE(FOnCardSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReviveTimerEnd);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCharacterInit, ACharacter*, AvatarCharacter);

UCLASS()
class AURA_API AAuraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAuraPlayerController();
	
public:
	UPROPERTY()
	FOnCharacterInit OnCharacterInit;
	
	FOnReviveTimerEnd OnReviveTimerEnd;
	
protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void AcknowledgePossession(APawn* P) override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnRep_PlayerState() override;
	
public:
	UFUNCTION()
	void CharacterInitialized(ACharacter* InCharacter);
	
	void AddAllMappingContexts();
	void RemoveAllMappingContexts();

public:
	UFUNCTION(Client, Reliable)
	void ShowDamageNumber(float DamageAmount, ACharacter* TargetCharacter, bool bBlockedHit, bool bCriticalHit, bool bHealed = false);

	// 범위 지정 데칼
	bool GetHitResultUnderMagicCircle(ECollisionChannel TraceChannel, bool bTraceComplex, FHitResult& HitResult) const;
	
	UFUNCTION(BlueprintCallable)
	void ShowMagicCircle(UMaterialInterface* DecalMaterial = nullptr, float InRange = 0.f, float InRadius = 200.f);

	UFUNCTION(BlueprintCallable)
	void HideMagicCircle();

	UFUNCTION(BlueprintCallable)
	const FVector GetMagicCircleLocation();
	
	// 범위 데칼
	UFUNCTION(BlueprintCallable)
	void ShowRangeIndicator(ERangeShape RangeShape, const FVector& Location, float Radius, float Width, float Height, FVector RGB);

	UFUNCTION(BlueprintCallable)
	void HideRangeIndicator();

	void SetTargetingStatus(ETargetingStatus InStatus);
	void SetCachedDestination(const FVector& InLocation){CachedDestination = InLocation;}
	TObjectPtr<USplineComponent> GetMoveSpline(){return Spline;}
	void SetAutoRunning(bool bInAuto){bAutoRunning = bInAuto;}

	bool IsShiftKeyDown() { return bShiftKeyDown; }
	FHitResult& GetHitResult() {return CursorHit;}

	UAuraAbilitySystemComponent* GetASC();
	
public:
	/*
	 * 어빌리티 업그레이드 카드
	 */
	// 카드 선택 UI 초기화 완료시 호출되는 델리게이트
	UFUNCTION()
	void HandleCardSelectionInitialized();
	
	// 카드 선택 버튼 콜백
	UFUNCTION()
	void HandleAbilityCardSelected(FGameplayTag SelectedUpgradeTag);
	
	UFUNCTION()
	void HandleAbilityInfoCardSelected(TArray<FAuraAbilityUpgradeInfo>& SelectedUpgradeInfo);

	// 리롤 버튼 클릭
	UFUNCTION()
	void HandleAbilityCardRerollSelected();
	
	UFUNCTION()
	void HandleInventoryUIInit();
	
public:
	/*
	//	서버 RPC 함수
	*/
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_TryPickUpItem(AAuraDropItem* DropItem, AAuraPlayerController* OwnerPC);
	
	UFUNCTION(Server, Reliable)
	void Server_TryRemoveItem(int32 SlotIndex);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_CreateCardSelection(AActor* InteractedActor);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RemoveCardSelection(AActor* InteractedActor);
	
	UFUNCTION()
	void SetAutoRunDestination(const FVector& Destination, const TArray<FVector>& PathPoints);

	// UFUNCTION(Server, Reliable)
	// void Server_StopAutoRun();
	
	// 선택된 업그레이드를 저장하도록 PlayerState로 보냄
	UFUNCTION(Server, Reliable)
	void Server_SelectUpgrade(FGameplayTag SelectedUpgradeTag);

	UFUNCTION(Server, Reliable)
	void Server_RemoveUpgrade(FGameplayTag RemoveTag);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_AddAbilityToPlayerByGameplayTag(const FGameplayTag& Tag);

	UFUNCTION(Server, Reliable)
	void Server_CharacterInvincible(bool bInvincible);
	
	UFUNCTION(Server, Reliable)
	void Server_CharacterDebugInvincible(bool bInvincible);
	
	UFUNCTION(Server, Reliable)
	void Server_CharacterInfiniteMana(bool bInfiniteMana);
	
	UFUNCTION(Client, Reliable)
	void SaveCharacterProgress(FName PlayerStartTag = FName());
	
	UFUNCTION(Server, Reliable)
	void Server_ReviveFromPlayerStart();
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestPauseGame(AAuraPlayerController* RequestedPC);

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RequestUnPauseGame(AAuraPlayerController* RequestedPC);
	
	UFUNCTION(Server, Reliable)
	void Server_ApplyWaitForExecuteTag();
	
	// 사망 후 관전
	UFUNCTION(Server, Reliable)
	void Server_StartSpectating();
	
	UFUNCTION(Server, Reliable)
	void Server_ApplyInputBlockTag();
	
	UFUNCTION(Server, Reliable)
	void Server_RemoveInputBlockTag();
	
	// 클라이언트 RPC 함수
	UFUNCTION(Client, Reliable)
	void Client_ShowGameOverWidget();

	UFUNCTION(Client, Unreliable)
	void Client_CreateMessageWidget(const FGameplayTag& MessageTag, const FText& AppendText, UTexture2D* Icon);
	
	UFUNCTION(Client, Reliable)
	void Client_RemoveCardSelection();
	
	UFUNCTION(Client, Reliable)
	void Client_OnBossEventStart(AActor* BossActor);

	UFUNCTION(Client, Reliable)
	void Client_OnBossEventEnd(AActor* BossActor);
	
	UFUNCTION(Client, Reliable)
	void Client_OnBossDead(AActor* BossActor);
	
	UFUNCTION(Client, Reliable)
	void Client_OnPickUpFinished();
	
protected: // 입력, 이동 함수
	void Move(const struct FInputActionValue& InputActionValue);
	void Zoom(const struct FInputActionValue& InputActionValue);
	void ActivateDebugMode(const struct FInputActionValue& InputActionValue);
	void ShowAttributeMenu();
	void ShowSpellMenu();
	void ShowESCMenu();
	void ShowInventoryMenu();
	void ShowItemTitle(const FInputActionValue& Value);
	
	void ShiftPressed() { bShiftKeyDown = true; }
	void ShiftReleased() { bShiftKeyDown = false; }

	void CursorTrace();
	
	void HighlightActor(AActor* InActor);
	void UnHighlightActor(AActor* InActor);

	// 어빌리티 입력
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);
	
	void AutoRun();

	UFUNCTION(BlueprintCallable)
	void AutoRunToLocation(const FVector& Location);

public:
	UFUNCTION(BlueprintCallable)
	void SetMoveToMouse(bool bAllow);
	
	UFUNCTION(BlueprintCallable)
	void AutoRunToActor(AActor* Actor);
	
protected: // 클릭, 이동 관련 변수 들
	// 마우스로 이동하기 여부 - 기본적으로 비활성화
	bool bAllowMoveToMouse = false;
	FHitResult CursorHit;
	TObjectPtr<AActor> LastActor;
	TObjectPtr<AActor> ThisActor;
	
	// 자동 달리기 허용 반경
	UPROPERTY(EditDefaultsOnly)
	float AutoRunAcceptanceRadius = 50.f;
	
	bool bDebugModeActivated = false;
	bool bShiftKeyDown = false;
	
	UPROPERTY(BlueprintReadOnly)
	FVector LastMagicCircleLocation;
	
	/* 클릭으로 이동 */
	FVector CachedDestination = FVector::ZeroVector;
	float FollowTime = 0.f;
	float ShortPressThreshold = 0.5f;
	bool bAutoRunning = false;
	ETargetingStatus TargetingStatus = ETargetingStatus::None;

	// 드랍 아이템 자동 이동 후 습득하기
	UPROPERTY()
	AActor* TargetItem = nullptr;
	
	UPROPERTY()
	bool bIsPickingUpItem = false;

public:
	UFUNCTION(BlueprintCallable)
	void StopAutoRun();

public:
	UFUNCTION()
	void BossMonsterBind();

	// 카메라 전환
	void ChangeCameraToBossActor(AActor* BossActor, float BlendTime, float ReturnTime);
	void ChangeCameraToOwn(float BlendTime);

	// 인풋 모드 변경
	void SetPlayerInputEnable(bool bEnable);
	
	void UpdateMagicCircleLocation();
	void UpdateRangeIndicatorRotation();

public:
	/* 튜토리얼 다이얼로그 UI */
	void ShowTutorialUI(bool bVisibility);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> AuraContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> MoveContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> MenuContext;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
	TObjectPtr<UInputMappingContext> PauseContext;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ShiftAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> WheelAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> DebugAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> AttributeMenuAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> SpellMenuAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ESCMenuAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> AltAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> InventoryMenuAction;

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UAuraInputConfig> InputConfig;

	UPROPERTY()
	TObjectPtr<UAuraAbilitySystemComponent> AuraAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> ClickNiagaraSystem;
	
protected:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UMVVM_TutorialDialogue> TutorialDialogueViewModel;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UUserWidget> TutorialDialogueViewClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> TutorialDialogueView;
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageTextComponent> DamageTextComponentClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AMagicCircle> MagicCircleClass;
	
	UPROPERTY()
	TObjectPtr<AMagicCircle> MagicCircle;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAbilityRangeIndicator> RangeIndicatorClass;
	
	UPROPERTY()
	TObjectPtr<AAbilityRangeIndicator> RangeIndicator;
};
