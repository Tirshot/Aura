// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actor/AuraArcaneMissile.h"
#include "AuraSpinningBeamActor.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API AAuraSpinningBeamActor : public AAuraProjectile
{
	GENERATED_BODY()
	
public:
	AAuraSpinningBeamActor();
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

public:
	void SetOrbitCenter(const FVector& InVector) { OrbitCenter = InVector; }
	void ApplyTickDamage();
	
public:
	UPROPERTY(Replicated, VisibleAnywhere, Category = "BeamSpell")
	float OrbitRadius = 100.f;
	
	UPROPERTY(Replicated, VisibleAnywhere, Category = "BeamSpell")
	float AngleSpeed = 500.f;
	
	UPROPERTY(Replicated, VisibleAnywhere, Category = "BeamSpell")
	float InitialAngle = 0.f;
	
	UPROPERTY(Replicated, VisibleAnywhere, Category = "BeamSpell")
	float DamageDeltaTime = 0.2f;

public:
	void SetOwnedAbility(UAuraDamageGameplayAbility* InAbility){ OwnedAbility = InAbility;}
	
protected:
	UPROPERTY()
	UAuraDamageGameplayAbility* OwnedAbility;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	USceneComponent* SceneComponent;
	
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TSubclassOf<AAuraSpinningBeamActor> BeamClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> BeamNiagara;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UNiagaraSystem> BeamImpactNiagara;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> LoopingSound;
	
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UAudioComponent> LoopingSoundComponent;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> ImpactSound;
	
	UPROPERTY(EditAnywhere, Category = "Orbit")
	FVector OrbitCenter = FVector::ZeroVector;
	
	float SpinningAngle = 0.f;
	float SelfSpinSpeed = 100.f;
	float TimeElapsed = 0.0f;
	float DestroyTimeElapsed = 0.0f;
};
