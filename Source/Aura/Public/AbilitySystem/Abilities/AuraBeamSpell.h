// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AuraBeamSpell.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraBeamSpell : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()
	
public:
	UAuraBeamSpell();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	UFUNCTION()
	void OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle);
	
	UFUNCTION()
	void OnMontageCompleted();
	
	UFUNCTION()
	void OnMontageInterrupted();
	
	UFUNCTION()
	void OnMontageTagReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	void OnInputReleased(float TimeHeld);
	
	UFUNCTION()
	void ApplyDamage();
	
	UFUNCTION(BlueprintCallable)
	void StoreMouseDataInfo(const FHitResult& InHitResult);

	UFUNCTION(BlueprintCallable)
	void StoreOwnerVariables();

	UFUNCTION(BlueprintCallable)
	void TraceFirstTarget(const FVector& BeamTargetLocation);

	UFUNCTION(BlueprintCallable)
	void StoreAdditionalTarget(TArray<AActor*>& OutAdditionalTargets);

	UFUNCTION()
	void PrimaryTargetDied(AActor* DeadActor);

	UFUNCTION()
	void AdditionalTargetDied(AActor* DeadActor);

protected:
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;
	
	UPROPERTY(BlueprintReadWrite, Category="Beam")
	FVector MouseHitLocation;

	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<AActor> MouseHitActor;

	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<APlayerController> OwnerPlayerController;

	UPROPERTY(BlueprintReadWrite, Category = "Beam")
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	float DamageDeltaTime = 0.1;
	
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	int32 MaxNumShockTargets = 5;

	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	int32 AdditionalTargets = 0;
	
	// 누름 유지 최소 시간
	UPROPERTY(EditDefaultsOnly, Category = "Beam")
	float MinSpellTime = 0.5f;
	
protected:
	bool bFirstTargetHasCombatInterface;
	FGameplayCueParameters FirstTargetCueParams;
	
	UPROPERTY()
	TArray<AActor*> AdditionalTargetActors;
	
	FTimerHandle DamageCostTimerHandle;
	
	FHitResult HitResult;
};
