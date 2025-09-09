
#pragma once

#include "CoreMinimal.h"
#include "AuraAbilityTypes.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "AuraArcaneArea.generated.h"

UCLASS()
class AURA_API AAuraArcaneArea : public AActor
{
	GENERATED_BODY()
	
public:	
	AAuraArcaneArea();

protected:
	virtual void BeginPlay() override;
	virtual void Destroyed() override;

	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	virtual void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
public:	
	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn = true))
	FDamageEffectParams DamageEffectParams;

	UFUNCTION()
	void ApplySlowEffect();

	UFUNCTION()
	void OnSlowStackChanged(FActiveGameplayEffectHandle ActiveGEHandle, int32 NewStackCount, int32 OldStackCount);

	bool IsValidOverlap(AActor* OtherActor);

	void DamageAndKnockback(AActor* OtherActor);

protected:
	UPROPERTY()
	TArray<AActor*> OverlappedActors;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> SlowDownEffectClass;

	UPROPERTY()
	FTimerHandle ApplyEffectTimer;
	
	UPROPERTY(EditDefaultsOnly, Category="ArcaneArea")
	float LifeSpan = 5.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ArcaneArea")
	float SlowSpeedRatio = 0.25f;
	
	UPROPERTY(EditDefaultsOnly, Category="ArcaneArea")
	float SlowRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category="ArcaneArea")
	float ApplyEffectPeriod = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category="ArcaneArea")
	bool bTakeDamage = false;

	FTimerHandle TimerHandle;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> Sphere;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UDecalComponent> Decal;


};
