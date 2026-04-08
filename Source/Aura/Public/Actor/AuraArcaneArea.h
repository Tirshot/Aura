
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

	bool IsValidOverlap(AActor* OtherActor);

	UFUNCTION()
	void DamageAndKnockback();

public:
	// 게터 or 세터
	void SetSlowSpeedRatio(float InSpeed) {SlowSpeedRatio = InSpeed;}
	void SetSlowRadius(float InSlowRadius) {SlowRadius = InSlowRadius;}
	void SetApplyEffectPeriod(float InPeriod) {ApplyEffectPeriod = InPeriod;}
	void SetTakeDamage(bool InBool) {bTakeDamage = InBool;}

public:
	// 슬로우 이펙트
	UPROPERTY()
	TSubclassOf<UGameplayEffect> SlowDownEffectClass;
	
	UPROPERTY(EditDefaultsOnly, Category="ArcaneArea")
	float LifeSpan = 5.f;
	
protected:
	UPROPERTY()
	TArray<AActor*> OverlappedActors;

	UPROPERTY()
	FTimerHandle ApplyEffectTimer;
	
	UPROPERTY()
	FTimerHandle ApplyDamageEffectTimer;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ArcaneArea")
	float SlowSpeedRatio = 0.25f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ArcaneArea")
	float SlowRadius = 250.f;

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
