// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CheckPoint/CheckPoint.h"
#include "AbilityUpgradeChest.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API AAbilityUpgradeChest : public ACheckPoint
{
	GENERATED_BODY()

public:
	AAbilityUpgradeChest(const FObjectInitializer& ObjectInitializer);

public:
	/* SaveInterface 함수 오버라이드 */
	virtual void LoadActor_Implementation() override;
	/* SaveInterface 오버라이트 끝 */

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame")
	FGuid Guid;
};
