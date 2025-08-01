// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MagicCircle.generated.h"

class UDecalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCircleInitialized, AActor*, AvatarActor);

UCLASS()
class AURA_API AMagicCircle : public AActor
{
	GENERATED_BODY()
	
public:	
	AMagicCircle();
	virtual void Tick(float DeltaTime) override;

	void KeepMagicCircleInRange();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDecalComponent> MagicCircleDecal;

	UPROPERTY(BlueprintAssignable)
	FOnCircleInitialized CircleInitialized;

	UPROPERTY(BlueprintAssignable)
	FOnCircleInitialized RemoveCircle;

	UPROPERTY(EditAnywhere)
	float CircleRange = 0.f;
	
	UPROPERTY(EditAnywhere)
	float Radius = 256.f;
	
protected:
	virtual void BeginPlay() override;
};
