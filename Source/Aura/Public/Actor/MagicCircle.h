// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/DecalComponent.h"
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
	
protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
	void KeepMagicCircleInRange();

public:
	void SetDecalSize(float InSize) {MagicCircleDecal->DecalSize = FVector(127.f, Radius, Radius);}
	void SetCircleRange(float InRange) {CircleRange = InRange;}
	void SetDecalMaterial(UMaterialInterface* InMaterial) {MagicCircleDecal->SetMaterial(0, InMaterial);}
	
	UPROPERTY(BlueprintAssignable)
	FOnCircleInitialized CircleInitialized;

	UPROPERTY(BlueprintAssignable)
	FOnCircleInitialized RemoveCircle;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDecalComponent> MagicCircleDecal;

	UPROPERTY(EditAnywhere)
	float CircleRange = 0.f;
	
	UPROPERTY(EditAnywhere)
	float Radius = 256.f;
};
