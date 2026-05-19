// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/AuraCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "Interaction/HighlightInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "AuraEnemy.generated.h"

class AMissionActor;
class UWidgetComponent;
class UBehaviorTree;
class AAuraAIController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChanged, float, NewValue, AActor*, InstigatorActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTotalReceivedDamageChanged, float, NewValue);

UCLASS()
class AURA_API AAuraEnemy : public AAuraCharacterBase, public IEnemyInterface, public IHighlightInterface
{
	GENERATED_BODY()
	
public:
	AAuraEnemy();
	
protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitAbilityActorInfo() override;
	virtual void InitializeDefaultAttributes() const override;
	virtual void MulticastHandleDeath(const FVector& DeathImpulse) override;
	
public:
	// 하이라이트 인터페이스 오버라이드
	virtual void HighlightActor_Implementation() override;
	virtual void UnHighlightActor_Implementation() override;
	virtual void SetMoveToLocation_Implementation(FVector& OutDestination) override;
	// 하이라이트 인터페이스 끝

	// 전투 인터페이스 오버라이드
	virtual int32 GetCharacterLevel_Implementation() override;
	virtual FOnDeath* GetOnDeathDelegate() override;
	virtual void Die(const FVector& DeathImpulse, AAuraCharacter* KilledBy) override;
	virtual void SetCombatTarget_Implementation(AActor* InCombatTarget) override;
	virtual AActor* GetCombatTarget_Implementation() const override;
	virtual void SetIsBeingShocked_Implementation(bool bInShock) override;
	virtual bool IsXPOverridden_Implementation() const override;
    virtual float GetXPOverriddenValue_Implementation() const override;
	virtual bool GetIgnoreKnockback_Implementation() override;
	virtual float GetTotalReceivedDamage_Implementation() override;
	virtual void AddTotalReceivedDamage_Implementation(float IncomingDamage) override;
	// 전투 인터페이스 끝
	
public:
	/*
	 *	델리게이트
	 */
	
	// 라이프 사이클, 데미지
	FOnDeath OnDeath;
	FOnDamageSignature OnDamageDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable)
	FOnAttributeChangedSignature OnMaxHealthChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnTotalReceivedDamageChanged OnTotalReceivedDamageChanged;
	
	// 콜백 함수
	virtual void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	virtual void StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount) override;
	virtual void BeingShockedTagChanged() override;
	
public:
	// Setter
	void SetLevel(int32 InLevel) {Level = InLevel;}
	void SetXPOverride(bool bOverride) {bIsXPOverride = bOverride;}
	void SetXPOverrideValue(float InXP) {XPOverrideValue = InXP;}
	void SetMissionActor(const TArray<AMissionActor*>& Actors) {MissionActors = Actors;}
	void AddMissionActor(AMissionActor* Actor) {MissionActors.Add(Actor);}

public:
	UPROPERTY(BlueprintReadOnly, category="Combat")
	bool bHitReacting = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category="Combat")
	bool bIsRangedAttacker = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category="Combat")
	bool bIgnoreReactTag = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category="Combat")
	bool bIgnoreKnockback = false;
	
	UPROPERTY(ReplicatedUsing="OnRep_TotalReceivedDamage", VisibleAnywhere, BlueprintReadOnly, category="Combat")
	float TotalReceivedDamage = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Combat")
	float LifeSpan = 5.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Combat")
	bool bIsXPOverride = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Combat", meta = (EditCondition = bIsXPOverride, EditConditionHides))
	float XPOverrideValue = 0.f;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, category = "Character Class Defaults")
	int32 Level = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Character Class Defaults")
	FString MonsterName = "Default Name";

	UPROPERTY(BlueprintReadWrite, Category="Combat")
	TObjectPtr<AActor> CombatTarget;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Mission")
	TArray<TObjectPtr<AMissionActor>> MissionActors;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> HealthBar;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> TotalReceivedDamageWidget;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture> PictureFrameImage;
	
	UPROPERTY(EditAnywhere, category="AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	UPROPERTY()
	TObjectPtr<AAuraAIController> AuraAIController;

public:
	// 아이템 드랍
	UFUNCTION(BlueprintImplementableEvent)
	void SpawnLoot();
	
public:
	UFUNCTION()
	void OnRep_TotalReceivedDamage();
	
	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void UpdateTotalReceivedDamageWidget(float TotalDamage);

public:
	/*
	 *  어빌리티 업그레이드
	 */
	// 어빌리티 업그레이드 및 스택 관리는 몬스터의 ASC에 직접
	void AddAbilityUpgrade(TSubclassOf<UGameplayEffect> AbilityUpgradeClass);
	void RemoveAbilityUpgrade(TSubclassOf<UGameplayEffect> AbilityUpgradeClass);

	// 추가 할 업그레이드 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UGameplayEffect>> AbilityUpgradeClassToApply;
};
