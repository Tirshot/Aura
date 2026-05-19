// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraAbilityTypes.h"
#include "GameFramework/Actor.h"
#include "AuraProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;

UCLASS()
class AURA_API AAuraProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AAuraProjectile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn = true))
	FDamageEffectParams DamageEffectParams;

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<USceneComponent> HomingTargetSceneComponent;

protected:
	virtual void BeginPlay() override;

	virtual void Destroyed() override;

	UFUNCTION(BlueprintCallable)
	virtual void OnHit();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Sphere;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void ApplyDamage(AActor* OtherActor);
	
	bool IsValidOverlap(AActor* OtherActor);

	UPROPERTY(BlueprintReadOnly)
	bool bHit = false;

protected:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag ImpactGameplayCue = FGameplayTag::EmptyTag;
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag LoopingGameplayCue = FGameplayTag::EmptyTag;
	
	UPROPERTY(EditDefaultsOnly)
	float LifeSpan = 15.f;
	
public:
	UPROPERTY()
	bool bCheckValidOverlap = true;
};
