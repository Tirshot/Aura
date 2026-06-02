// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InstantKillActor.generated.h"

class UGameplayEffect;
class UBoxComponent;

UCLASS()
class AURA_API AInstantKillActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AInstantKillActor();

protected:
	virtual void BeginPlay() override;
	
	// 낙사 구역을 담당할 큰 박스 콜리전
	UPROPERTY(VisibleAnywhere, Category = "KillZone")
	TObjectPtr<UBoxComponent> KillBox;

	// 충돌 시작 함수
	UFUNCTION()
	void OnKillBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
						 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, 
						 bool bFromSweep, const FHitResult& SweepResult);
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> InstantDeathGEClass;

};
