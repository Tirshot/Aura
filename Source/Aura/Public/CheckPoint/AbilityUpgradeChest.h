// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CheckPoint/CheckPoint.h"
#include "Interaction/ItemInterface.h"
#include "AbilityUpgradeChest.generated.h"

UCLASS()
class AURA_API AAbilityUpgradeChest : public ACheckPoint, public IItemInterface
{
	GENERATED_BODY()

public:
	AAbilityUpgradeChest(const FObjectInitializer& ObjectInitializer);
	virtual void PostNetInit() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	/* SaveInterface 함수 오버라이드 */
	virtual void LoadActor_Implementation() override;
	/* SaveInterface 오버라이트 끝 */
	
	/* ItemInterface */
	virtual FGuid GetGuid_Implementation() override;
	virtual bool IsReached_Implementation() override;
	/* ItemInterface 끝 */

protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_Reached() override;
	
public:
	void UpdateChestState();
	
	UFUNCTION(BlueprintCallable)
	void OnTimelineAnimationFinished(AActor* InteractedActor);
	
public:
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	
protected:
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SaveGame")
	FGuid Guid;
};
