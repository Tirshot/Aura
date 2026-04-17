// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/ItemInfo.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "Interaction/HighlightInterface.h"
#include "Interaction/ItemInterface.h"
#include "AuraDropItem.generated.h"

class UWidgetComponent;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropItemInitialized, FText, ItemTitle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClickedDroppedItem, AActor*, ItemOwner);

UCLASS()
class AURA_API AAuraDropItem : public AActor, public IHighlightInterface, public IItemInterface
{
	GENERATED_BODY()
	
public:	
	AAuraDropItem();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
public:
	// 하이라이트 인터페이스 오버라이드
	virtual void HighlightActor_Implementation() override;
	virtual void UnHighlightActor_Implementation() override;
	virtual void SetMoveToLocation_Implementation(FVector& OutDestination) override;
	void SetMeshAndMaterial();
	// 하이라이트 인터페이스 끝
	
public:
	void InitializeItem(const FItemData& InItemData);
	
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult);
	
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UFUNCTION(BlueprintCallable)
	void SetItemCount(int32 InCount) {ItemCount = InCount;}
	
	void SetTitleWidgetVisibility(bool InValue);
	
	UFUNCTION()
	void OnRep_DropItemData();
	
	UFUNCTION()
	void OnRep_IsPickedUp();
	
	UPROPERTY(BlueprintAssignable)
	FOnDropItemInitialized OnDropItemInitialized;
	
	FOnClickedDroppedItem OnClickedDroppedItem;
	
public:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USphereComponent> Sphere;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UWidgetComponent> ItemTitleWidget;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> MoveToComponent;
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DropItemData)
	FItemData DropItemData;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FGuid UniqueID;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Category="ItemData"))
	FDataTableRowHandle ItemHandle;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Category="ItemData"))
	int32 ItemCount = 1;
	
	// 서버 RPC에 의해 줍고 있는 중인지 판단하는 불리언
	UPROPERTY(ReplicatedUsing = OnRep_IsPickedUp)
	bool bIsPickedUp = false;
};
