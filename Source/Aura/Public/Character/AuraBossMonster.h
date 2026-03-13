// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Character/AuraEnemy.h"
#include "AuraBossMonster.generated.h"

class USpringArmComponent;
class UCameraComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossEventStart, AActor*, BossActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBossEventEnd, AActor*, BossActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBeginBerserkMontage);

UCLASS()
class AURA_API AAuraBossMonster : public AAuraEnemy
{
	GENERATED_BODY()

public:
	AAuraBossMonster();
	void BossMontageBind();
	
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	UPROPERTY()
	FOnBossEventStart OnBossEventStart;
	
	UPROPERTY()
	FOnBossEventEnd OnBossEventEnd;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnBeginBerserkMontage OnBeginBerserkMontage;
	
public:
	// 전투 인터페이스 오버라이드
	virtual void Die(const FVector& DeathImpulse, AAuraCharacter* KilledBy) override;
	virtual void SetIsBeingShocked_Implementation(bool bInShock) override;
	// 전투 인터페이스 끝

	UFUNCTION()
	void ChangeGlobalTimeDilationToDefault();
	
public:
	virtual void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	virtual void StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount) override;
	virtual void BeingShockedTagChanged() override;

	void OnRoarStart(const FGameplayEventData* EventData);
	void OnRoarEnd(const FGameplayEventData* EventData);
	void AddAbilityUpgradeOnBerserkMode();

public:
	UFUNCTION()
	void BeginBerserkMode(float NewHealth);
	
	UFUNCTION()
	void OnRep_IsRoaring();

	UPROPERTY(ReplicatedUsing = OnRep_IsRoaring)
	bool bIsRoaring = false;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float BeginBerserkRatio = 0.5f;
	
	UPROPERTY(VisibleAnywhere, Category="Camera")
	TObjectPtr<USpringArmComponent> SpringArm;
	
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> DeathCamera;

	// 광폭화 시 추가 할 업그레이드 태그
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UGameplayEffect>> AbilityUpgradeClassToApplyBerserkMode;
};
