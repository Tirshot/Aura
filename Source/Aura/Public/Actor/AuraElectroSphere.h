// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actor/AuraProjectile.h"
#include "AuraElectroSphere.generated.h"

UCLASS()
class AURA_API AAuraElectroSphere : public AAuraProjectile
{
	GENERATED_BODY()

public:
	AAuraElectroSphere();
	
protected:
	virtual void BeginPlay() override;
	virtual void OnHit() override;

	virtual void Destroyed() override;

	UFUNCTION()
	void ApplyMainDamage();

	UFUNCTION(BlueprintCallable)
	void DetectAdditionalTargets(TArray<AActor*> ActorsToIgnore);

	UFUNCTION(BlueprintCallable)
	void HomingNearestTarget();
	
public:
	void SetDamageDeltaSecond(float InTime) {DamageDeltaSecond = InTime;}
	void SetDamageRadius(float InDamageRadius) {DamageRadius = InDamageRadius;}
	void SetMovementSpeed(float InSpeed);
	void AddMovementSpeed(float InSpeed);
	void SetTraceRadius(float InTraceRadius) {TraceRadius = InTraceRadius;}
	void SetFollowToTarget(bool bFollow) {bHomingTarget = bFollow;}

	void SetAdditionalTargets(int32 InNum) {AdditionalTargets = InNum;}
	void SetMaxNumShockTargets(int32 InNum) {MaxNumShockTargets = InNum;}

	void SetHomingTarget(bool bHoming) {bHomingTarget = bHoming;}

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Mesh;
	
private:
	UPROPERTY(EditDefaultsOnly)
	int32 AdditionalTargets = 2;
	
	UPROPERTY(EditDefaultsOnly)
	int32 MaxNumShockTargets = 5;
	
	UPROPERTY(EditDefaultsOnly)
	float MovementSpeed = 500.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float DamageDeltaSecond = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	bool bHomingTarget = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float TraceRadius = 600.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	float DamageRadius = 200.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess = true))
	TArray<AActor*> MainOverlappingActors;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess = true))
	TArray<AActor*> AdditionalOverlappingActors;
	
	FTimerHandle TimerHandle;
};
