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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_NineParams(FOnIndicatorInitialized, AActor*, AvatarActor, bool, bAttachToActor, ERangeShape, RangeShape, const FVector&, Location, float, Radius, float, Width, float, Height, float, Angle, const FVector&, RGB);
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

	UFUNCTION()
	void ShowIndicator(AActor* AvatarActor, bool bAttachToActor, ERangeShape Shape, const FVector& Location, float InRadius = 0.f, float InWidth = 0.f, float InHeight = 0.f, float InAngle = 0.f, const FVector& RGB = FVector(5,5,5));

	ERangeShape GetRangeShape() const { return RangeShape; }
	float GetWidth() const { return Width; }
	float GetHeight() const { return Height; }
	
protected:
	// 범위 표시기 모양
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	ERangeShape IndicatorShape;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UDecalComponent> DecalComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInstance> CircleMaterial;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UMaterialInstance> RectMaterial;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMI;

public:
	UPROPERTY()
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

	UPROPERTY(BlueprintReadOnly, meta=(AllowPrivateAccess = true))
	ERangeShape RangeShape;
};
