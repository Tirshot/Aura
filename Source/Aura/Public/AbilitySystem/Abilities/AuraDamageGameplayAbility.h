// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraAbilityTypes.h"
#include "AuraDamageGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraDamageGameplayAbility : public UAuraGameplayAbility
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void CauseDamage(AActor* TargetActor);

	UFUNCTION(BlueprintPure)
	FDamageEffectParams MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor = nullptr) const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	FGameplayTag DamageType;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	FScalableFloat Damage;

	// 디버프 멤버변수
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffChance = 20.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffDamage = 5.f;

	// 몇 초마다 디버프 데미지를 입힐 것인가
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffFrequency = 1.f;

	// 몇 초동안 디버프를 유지할 것인가
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffDuration = 5.f;

	// 랙돌에 가할 충격파 크기
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DeathImpulseMagnitude = 1000.f;

	// 넉백 크기
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float KnockbackForceMagnitude = 1000.f;

	// 넉백 확률
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float KnockbackChance = 0.f;

	UFUNCTION(BlueprintPure)
	FTaggedMontage GetRandomTaggedMontageFromArray(const TArray<FTaggedMontage>& TaggedMontages) const;

};
