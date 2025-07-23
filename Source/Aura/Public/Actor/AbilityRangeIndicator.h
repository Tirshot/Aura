// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilityRangeIndicator.generated.h"

UENUM(BlueprintType)
enum class ERangeShape : uint8
{
	ERS_Circle = 0, // 원형
	ERS_Rectangle, // 직사각형
	ERS_Cone, // 부채꼴
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_NineParams(FOnIndicatorInitialized, AActor*, AvatarActor, ERangeShape, RangeShape, const FVector&, Location, float, Width, float, Height, float, Radius, float, Red, float, Green, float, Blue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIndicatorRemoved, AActor*, AvatarActor);

UCLASS()
class AURA_API AAbilityRangeIndicator : public AActor
{
	GENERATED_BODY()
	
public:	
	AAbilityRangeIndicator();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	void ShowIndicator(ERangeShape Shape, const FVector& Location, float InRadius = 0.f, float InWidth = 0.f, float InHeight = 0.f, float InAngle = 0.f);

protected:
	// 범위 표시기 모양
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ERangeShape IndicatorShape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDecalComponent> DecalComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInterface> BaseMaterial;

public:
	UPROPERTY(BlueprintAssignable)
	FOnIndicatorInitialized IndicatorInitialized;

	UPROPERTY(BlueprintAssignable)
	FOnIndicatorRemoved RemoveIndicator;
	
private:
	// 원형, 부채꼴
	float Radius = 0.f;

	// 직사각형
	float Width = 0.f;
	float Height = 0.f;

	// 부채꼴 각도
	float ConeAngle = 0.f;
};
