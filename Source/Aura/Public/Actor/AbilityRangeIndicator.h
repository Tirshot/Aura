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
	ERS_RectangleAndCircle,
	ERS_Cone, // 부채꼴
};

USTRUCT()
struct FRangeIndicatorParams
{
	GENERATED_BODY()

	UPROPERTY()
	bool bAttachToActor = false;

	UPROPERTY()
	ERangeShape RangeShape = ERangeShape::ERS_Circle;

	UPROPERTY()
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY()
	float Radius = 0.f;

	UPROPERTY()
	float Width = 0.f;

	UPROPERTY()
	float Height = 0.f;

	UPROPERTY()
	float ConeAngle = 0.f;

	UPROPERTY()
	FVector IndicatorColor = FVector::ZeroVector;
};

UCLASS()
class AURA_API AAbilityRangeIndicator : public AActor
{
	GENERATED_BODY()
	
public:	
	AAbilityRangeIndicator();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	//UFUNCTION()
	//void ShowIndicator(AActor* AvatarActor, bool bAttachToActor, ERangeShape Shape, const FVector& StartLocation, float InRadius = 0.f, float InWidth = 0.f, float InHeight = 0.f, float InAngle = 0.f, const FVector& RGB = FVector(5,5,5));
	void UpdateDecalVisual();

	void ShowIndicatorOwnerOnly();
	
	UFUNCTION()
	void InitializeIndicatorParams(AActor* InAvatarActor, bool bInAttachToActor, ERangeShape InShape, const FVector& InStartLocation, float InRadius, float InWidth, float InHeight, float InAngle, const FVector& InRGB);
	
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
	UPROPERTY(ReplicatedUsing = OnRep_RangeParams)
	FRangeIndicatorParams RangeParams;
	
	UFUNCTION()
	void OnRep_RangeParams();

	// 실제 비주얼과 스케일을 업데이트하는 핵심 함수
	void InitIndicatorVisual();
	
	UFUNCTION()
	void OnRep_IndicatorColor();
	
	virtual void OnRep_Owner() override;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector RotateStartLocation;
	
	UPROPERTY(ReplicatedUsing = OnRep_IndicatorColor)
	FVector IndicatorColor = FVector::ZeroVector;
	
public:
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite)
	bool bRotate = false;
	
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadWrite)
	float RotationSpeed = 90.f;
};
