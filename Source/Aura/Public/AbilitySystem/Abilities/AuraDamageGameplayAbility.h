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
	FDamageEffectParams MakeDamageEffectParamsFromClassDefaults(
		AActor* TargetActor = nullptr,
		FVector InRadialDamageOrigin = FVector::ZeroVector,
		bool bOverrideKnockbackDirection = false,
		FVector InKnockbackDirectionOverride = FVector::ZeroVector,
		bool bOverrideDeathImpulse = false,
		FVector DeathImpulseDirectionOverride = FVector::ZeroVector,
		bool bOverridePitch = false,
		float PitchOverride = 0.f
	) const;

	UFUNCTION(BlueprintPure)
	float GetDamageAtLevel() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	FGameplayTag DamageType;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	FScalableFloat Damage;

	// ����� �������
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffChance = 20.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffDamage = 5.f;

	// �� �ʸ��� ����� �������� ���� ���ΰ�
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffFrequency = 1.f;

	// �� �ʵ��� ������� ������ ���ΰ�
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffDuration = 5.f;

	// ������ ���� ����� ũ��
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DeathImpulseMagnitude = 1000.f;

	// �˹� ũ��
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float KnockbackForceMagnitude = 1000.f;

	// �˹� Ȯ��
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float KnockbackChance = 0.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	bool bIsRadialDamage = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = "Damage")
	float RadialDamageInnerRadius = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = "Damage")
	float RadialDamageOuterRadius = 0.f;

	UFUNCTION(BlueprintPure)
	FTaggedMontage GetRandomTaggedMontageFromArray(const TArray<FTaggedMontage>& TaggedMontages) const;

};
