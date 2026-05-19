// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Actor.h"
#include "ProjectileSpawner.generated.h"

class AAuraEnemy;
struct FDamageEffectParams;
class AAuraProjectile;
class UGameplayEffect;

UCLASS()
class AURA_API AProjectileSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectileSpawner();

protected:
	virtual void BeginPlay() override;

	void SpawnProjectile();
	
	FDamageEffectParams MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor = nullptr) const;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	TSubclassOf<AAuraProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Spawner")
	float SpawnInterval = 2.0f;
	
protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	TObjectPtr<AAuraEnemy> DummyEnemy;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	FGameplayTag DamageType;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float Damage;
	
	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffChance = 20.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffDamage = 5.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffFrequency = 1.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float DebuffDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float KnockbackForceMagnitude = 1000.f;

	UPROPERTY(EditDefaultsOnly, category = "Damage")
	float KnockbackChance = 0.f;
	
private:
	FTimerHandle SpawnTimerHandle;
    
	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneComp;
};
