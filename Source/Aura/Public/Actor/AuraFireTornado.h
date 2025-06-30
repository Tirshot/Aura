// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actor/AuraProjectile.h"
#include "AuraFireTornado.generated.h"

class UCapsuleComponent;

UCLASS()
class AURA_API AAuraFireTornado : public AActor
{
	GENERATED_BODY()

	AAuraFireTornado();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;

public:
	void SetDamageDeltaSecond(float InTime) {DamageDeltaSecond = InTime;}
	void SetDamageRadius(float InDamageRadius) {DamageRadius = InDamageRadius;}
	void SetFollowRadius(float InFollowRadius) {FollowRadius = InFollowRadius;}

protected:
	UFUNCTION()
	void ApplyDamageAndKnockback();
	
	bool IsValidOverlap(AActor* OtherActor);

public:
	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn = true))
	FDamageEffectParams DamageEffectParams;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCapsuleComponent> Capsule;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAudioComponent> LoopingSoundComponent;
	
private:
	UPROPERTY(EditDefaultsOnly)
	float LifeSpan = 15.f;
	
	UPROPERTY(EditDefaultsOnly)
	float MovementSpeed = 1.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float DamageDeltaSecond = 0.2f;

	UPROPERTY(EditDefaultsOnly)
	float FollowRadius = 600.f;
	
	UPROPERTY(EditDefaultsOnly)
	float DamageRadius = 300.f;

	UPROPERTY(EditDefaultsOnly)
	float SpinDegreePerSecond = -360.f;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<USoundBase> LoopingSound;

	UPROPERTY(BlueprintReadWrite, meta=(AllowPrivateAccess = true))
	TArray<AActor*> OverlappingActors;

	FTimerHandle TimerHandle;
};
