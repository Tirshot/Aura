// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraGameplayAbility.h"
#include "AuraAbilityTypes.h"
#include "AuraDamageGameplayAbility.generated.h"

class AAbilityRangeIndicator;
enum class ERangeShape : uint8;

UENUM(BlueprintType)
enum ESpellType : uint8
{
	NonTargeting,
	Targeting,
	Ranged,
	Projectile
};


UCLASS()
class AURA_API UAuraDamageGameplayAbility : public UAuraGameplayAbility
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayAbility")
	float DefaultAbilityRange = 300.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayAbility")
	float AbilityRange = 300.f;

	UFUNCTION(BlueprintCallable)
	void CauseDamage(AActor* TargetActor);

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UFUNCTION(BlueprintCallable)
	void StopAutoRun();
	
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

	UFUNCTION(BlueprintCallable)
	void RemoveRangeSpellHelpMessage(AActor* AvatarActor);
	
	UFUNCTION(BlueprintCallable)
	void ShowMagicCircle(float MagicCircleRadius);
	
	UFUNCTION(BlueprintCallable)
	void ShowRangeIndicator(ERangeShape Shape, float Width = 0.f);
	
	UFUNCTION(BlueprintCallable)
	void HideMagicCircleAndRangeIndicator();

	UFUNCTION(BlueprintCallable)
	AAbilityRangeIndicator* SpawnRangeIndicator(const FVector& Location, bool bAttachToActor,ERangeShape RangeShape, float Radius, float Width, float Height, FVector RGB, float LifeSpan);
	
	UFUNCTION(BlueprintCallable)
	FVector ReceivedMouseHitResult(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	FGameplayTag DamageType;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	FScalableFloat Damage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	FScalableFloat MagicPowerCoefficient;
	
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffChance = 20.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffDamage = 5.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffFrequency = 1.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DeathImpulseMagnitude = 1000.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float KnockbackForceMagnitude = 1000.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float KnockbackChance = 0.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	bool bIsRadialDamage = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = "Damage")
	float RadialDamageInnerRadius = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category = "Damage")
	float RadialDamageOuterRadius = 0.f;

	UPROPERTY(BlueprintReadWrite)
	FVector CurrentTargetLocation = FVector::ZeroVector;
	
	UFUNCTION(BlueprintPure)
	FTaggedMontage GetRandomTaggedMontageFromArray(const TArray<FTaggedMontage>& TaggedMontages) const;

public:
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ESpellType> SpellType = ESpellType::NonTargeting;
};
