// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actor/AuraProjectile.h"
#include "AuraFireBall.generated.h"

UCLASS()
class AURA_API AAuraFireBall : public AAuraProjectile
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintImplementableEvent)
	void StartOutgoingTimeline();
	
	void SetShowBlastIndicator(bool bShow) {bShowBlastIndicator = bShow;}
	void SetExplodeAtMaxRange(bool bExplode) {bExplodeAtMaxRange = bExplode;}
	void SetTravelDistance(float Distance) {TravelDistance = Distance;}
	void SetExplodeDistance(float Distance) {ExplodeDistance = Distance;}

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> ReturnToActor;
	
	UPROPERTY(BlueprintReadWrite)
	FDamageEffectParams ExplosionDamageParams;

protected:
	virtual void BeginPlay() override;
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	
	virtual void OnHit() override;

protected:
	UPROPERTY(BlueprintReadWrite)
	FVector FinalLocation;
	
	UPROPERTY(BlueprintReadWrite)
	FVector ApexLocation;
	
	UPROPERTY(BlueprintReadWrite)
	float ExplodeDistance = 150.f;
	
	UPROPERTY(BlueprintReadWrite)
	float TravelDistance = 800.f;
	
	UPROPERTY(EditDefaultsOnly)
	bool bShowBlastIndicator = false;
	
	UPROPERTY(BlueprintReadOnly)
	bool bExplodeAtMaxRange = false;
};
