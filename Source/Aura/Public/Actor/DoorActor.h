// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/Actor.h"
#include "DoorActor.generated.h"

UCLASS()
class AURA_API ADoorActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ADoorActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
public:	
	UFUNCTION(BlueprintCallable)
	void OpenDoor();
	
	UFUNCTION()
	void OnRep_IsOpen();

protected:
	// 타임라인 관련
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door")
	UCurveFloat* DoorCurve;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door")
	float OpenAngle = 90.f;

	UPROPERTY(ReplicatedUsing=OnRep_IsOpen)
	bool bIsOpen = false;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Mesh;

private:
	FTimeline DoorTimeline;

	// 시작/끝 회전값
	FRotator InitialRotation;
	FRotator TargetRotation;

	UFUNCTION()
	void OnDoorTimelineTick(float Value);

	void PlayOpenTimeline();
};
