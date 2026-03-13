// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Aura/Aura.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/HighlightInterface.h"
#include "Interaction/SaveInterface.h"
#include "CheckPoint.generated.h"

class USphereComponent;

UCLASS()
class AURA_API ACheckPoint : public APlayerStart, public ISaveInterface, public IHighlightInterface
{
	GENERATED_BODY()
	
public:
	ACheckPoint(const FObjectInitializer& ObjectInitializer);

protected:
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintImplementableEvent)
	void CheckPointReached(UMaterialInstanceDynamic* InDynamicMI, AActor* InteractedActor);

	UFUNCTION(BlueprintCallable)
	void HandleGlowEffects(AActor* InteractedActor = nullptr);
	
public:
	/* HighlightInterface 오버라이드*/
	virtual void HighlightActor_Implementation() override;
	virtual void UnHighlightActor_Implementation() override;
	virtual void SetMoveToLocation_Implementation(FVector& OutDestination) override;
	/* HighlightInterface 끝*/

	/* SaveInterface 함수 오버라이드 */
	virtual bool ShouldLoadTransform_Implementation() override { return false; }
	virtual void LoadActor_Implementation() override;
	/* SaveInterface 오버라이트 끝 */
	
protected:
	UFUNCTION()
	virtual void OnRep_Reached();
	
public:
	UPROPERTY(EditDefaultsOnly)
	int32 CustomDepthStencilOverride = CUSTOM_DEPTH_TAN;

	// 활성화 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bActivated = true;
	
	// 이미 도달했는가
	UPROPERTY(ReplicatedUsing=OnRep_Reached, BlueprintReadWrite, SaveGame)
	bool bReached = false;

	// 접촉 시 힐링 여부
	UPROPERTY(EditDefaultsOnly)
	bool bHealing = true;

	// 게임 세이브 여부
	UPROPERTY(EditDefaultsOnly)
	bool bSaveGame = true;
	
	UPROPERTY(EditAnywhere)
	bool bBindOverlapCallback = true;

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<class UGameplayEffect> AuraHeal;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> CheckpointMesh;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USphereComponent> Sphere;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> MoveToComponent;
	
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMI;
};
