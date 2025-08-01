// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "MoveToAbilityRange.generated.h"

class AAuraCharacter;
class AAuraPlayerController;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnReachedAbilityRange);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMoveFailed);

UCLASS()
class AURA_API UMoveToAbilityRange : public UAbilityTask
{
	GENERATED_BODY()

public:
	UMoveToAbilityRange();

	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UMoveToAbilityRange* MoveToAbilityRange(
		UGameplayAbility* OwningAbility,
		const FVector TargetLocation,
		float AbilityRange,
		AActor* AvatarActor = nullptr);

	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

	void RangeCheck();
	void MoveToTargetLocation();

	UPROPERTY(BlueprintAssignable)
	FOnReachedAbilityRange OnReached;
	
	UPROPERTY(BlueprintAssignable)
	FOnMoveFailed OnMoveFailed;

protected:
	UPROPERTY()
	float AbilityRange;

	UPROPERTY()
	FVector TargetLocation;

	UPROPERTY()
	TWeakObjectPtr<AActor> AvatarActor;

	UPROPERTY()
	TWeakObjectPtr<AAuraCharacter> AuraCharacter;

	UPROPERTY()
	TWeakObjectPtr<AAuraPlayerController> AuraPC;
	
	
};
