// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CheckPoint/CheckPoint.h"
#include "MapEntrance.generated.h"

UCLASS()
class AURA_API AMapEntrance : public ACheckPoint
{
	GENERATED_BODY()

public:
	AMapEntrance(const FObjectInitializer& ObjectInitializer);
	
	/* HighlightInterface 오버라이드*/
	virtual void HighlightActor_Implementation() override;
	/* HighlightInterface 끝*/
	
	/* SaveInterface 함수 오버라이드 */
	virtual void LoadActor_Implementation() override;
	/* SaveInterface 오버라이트 끝 */

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UWorld> DestinationMap;

	UPROPERTY(EditAnywhere)
	FName DestinationPlayerStartTag;
	
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;
	
};