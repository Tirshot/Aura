// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AuraSpinningBeam.generated.h"

class AAuraSpinningBeamActor;
/**
 * 
 */
UCLASS()
class AURA_API UAuraSpinningBeam : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()
	
	// 보스 용 패턴 - 보스를 중심으로 각속도로 회전하는 어빌리티
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
public:
	UFUNCTION(BlueprintCallable)
	void SpawnBeam(float StartAngle);
	
	UFUNCTION(BlueprintImplementableEvent)
	void BeamDestroyed();
	
	
protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraSpinningBeamActor> BeamClass;

	UPROPERTY(BlueprintReadOnly)
	TArray<AAuraSpinningBeamActor*> LightningBeams;
	
	UPROPERTY(BlueprintReadWrite)
	TArray<FRotator> SavedRotators;
	
	// 회전 횟수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxRotations = 3.f;
	
	// 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float SpreadDegree = 360.f;
	
	// 회전 속도(각속도, per sec)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float AngleSpeed = 90.f;
	
	// 빔 갯수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 BeamCount = 6;
	
	// 최대 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float OrbitRadius = 800.f;
	
	// 데미지 간격
	UPROPERTY(EditDefaultsOnly)
	float DamageDeltaTime = 0.2f;
};
