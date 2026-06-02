// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AuraProjectile.h"
#include "GameFramework/Actor.h"
#include "ChainedLightningActor.generated.h"

class UNiagaraSystem;
class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class AURA_API AChainedLightningActor : public AAuraProjectile
{
	GENERATED_BODY()
	
public:	
	AChainedLightningActor();
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;
	
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	
public:
	void SetChainRadius(float InRadius) {ChainRadius = InRadius;}
	void SetMaxChain(float InMaxChain) {MaxChain = InMaxChain;}
	void SetDamageReductionRatio(float InRatio) {DamageReductionRatio = InRatio;}
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USceneComponent> CueStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<USceneComponent> CueEnd;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> StartSphere;
	
	UPROPERTY()
	TArray<AActor*> ActorsToIgnore;
	
	UPROPERTY()
	float DamageReductionRatio = 0.667f;
	
	UPROPERTY()
	float Length = 500.f;
	
	// 반사 반경
	float ChainRadius = 500.f;
	
	// 반사 횟수
	int32 MaxChain = 5;
	int32 CurrentChain = 1;
	
	bool bTailing = false;
};
