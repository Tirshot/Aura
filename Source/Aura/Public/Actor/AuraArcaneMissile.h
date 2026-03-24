// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actor/AuraProjectile.h"
#include "AuraArcaneMissile.generated.h"

class UAuraDamageGameplayAbility;
class UAuraArcaneOrbit;
/**
 * 
 */
UCLASS()
class AURA_API AAuraArcaneMissile : public AAuraProjectile
{
	GENERATED_BODY()

public:
	AAuraArcaneMissile();
	
	UPROPERTY(BlueprintReadWrite)
	FDamageEffectParams ExplosionDamageParams;

public:
	virtual void SetOrbitCenter(const FVector& InVector) { OrbitCenter = InVector; }
	virtual void SetHasTarget(bool InBool) {TargetSet = InBool;}
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	void HomingNearestTarget(float DeltaTime);
	
public:
	UPROPERTY(Replicated, VisibleAnywhere, Category = "Orbit")
	float OrbitRadius = 100.f;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Orbit")
	float OrbitSpeed = 500.f;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Orbit")
	float InitialAngle = 0.f;
	
	UPROPERTY(VisibleAnywhere, Category = "Orbit")
	float FollowRadius = 1000.f;
	
	UPROPERTY(EditAnywhere, Category = "Orbit")
	float InitialDelayDuration = 1.5f;

public:
	void SetOwnedAbility(UAuraDamageGameplayAbility* InAbility){ OwnedAbility = InAbility;}
	
protected:	
	UPROPERTY()
	UAuraDamageGameplayAbility* OwnedAbility;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, Category = "Orbit")
	FVector OrbitCenter = FVector::ZeroVector;
	
	float SpinningAngle = 0.f;

	float SelfSpinSpeed = 100.f;

	bool TargetSet = false;
	
	float TimeElapsed = 0.0f;
};
