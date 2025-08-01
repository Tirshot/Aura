// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayTagContainer.h"
#include "Actor/AbilityRangeIndicator.h"
#include "Actor/MagicCircle.h"
#include "AuraPlayerController.generated.h"

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
	None
};

DECLARE_MULTICAST_DELEGATE(FOnCardSelected);

UCLASS()
class AURA_API AAuraPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAuraPlayerController();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

public:
	UFUNCTION(Client, Reliable)
	void ShowDamageNumber(float DamageAmount, ACharacter* TargetCharacter, bool bBlockedHit, bool bCriticalHit, bool bHealed = false);

	// 범위 지정 데칼
	UFUNCTION(BlueprintCallable)
	void ShowMagicCircle(UMaterialInterface* DecalMaterial = nullptr, float InRange = 0.f, float InRadius = 200.f);

	UFUNCTION(BlueprintCallable)
	void HideMagicCircle();

	UFUNCTION(BlueprintCallable)
	const FVector GetMagicCircleLocation() { return MagicCircle->GetActorLocation(); }
	
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

	
public:
	/*
	 * 어빌리티 업그레이드 카드
	 */
	// 카드 선택 UI 초기화 완료시 호출되는 델리게이트
	FOnCardSelected OnCardSelectedDelegate;

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
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_CreateCardSelection(AActor* InteractedActor);
	
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_RemoveCardSelection(AActor* InteractedActor);
	
	// 선택된 업그레이드를 저장하도록 PlayerState로 보냄
	UFUNCTION(Server, Reliable)
	void Server_SelectUpgrade(FGameplayTag SelectedUpgradeTag);

	UFUNCTION(Server, Reliable)
	void Server_RemoveUpgrade(FGameplayTag RemoveTag);

private:
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputMappingContext> AuraContext;
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> ShiftAction;

	void Move(const struct FInputActionValue& InputActionValue);
	void ShiftPressed() { bShiftKeyDown = true; }
	void ShiftReleased() { bShiftKeyDown = false; }
	bool bShiftKeyDown = false;

	void CursorTrace();
	FHitResult CursorHit;
	TObjectPtr<AActor> LastActor;
	TObjectPtr<AActor> ThisActor;
	static void HighlightActor(AActor* InActor);
	static void UnHighlightActor(AActor* InActor);

	// 어빌리티 입력
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);

	UPROPERTY(EditDefaultsOnly, Category="Input")
	TObjectPtr<UAuraInputConfig> InputConfig;

	UPROPERTY()
	TObjectPtr<UAuraAbilitySystemComponent> AuraAbilitySystemComponent;

	UAuraAbilitySystemComponent* GetASC();

	/* 클릭으로 이동 */
	FVector CachedDestination = FVector::ZeroVector;
	float FollowTime = 0.f;
	float ShortPressThresold = 0.5f;
	bool bAutoRunning = false;
	ETargetingStatus TargetingStatus = ETargetingStatus::None;

	// 자동 달리기 허용 반경
	UPROPERTY(EditDefaultsOnly)
	float AutoRunAcceptanceRadius = 50.f;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UNiagaraSystem> ClickNiagaraSystem;

	void AutoRun();

public:
	UFUNCTION(BlueprintCallable)
	void StopAutoRun();

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
	
	void UpdateMagicCircleLocation();
	void UpdateRangeIndicatorRotation();
};
