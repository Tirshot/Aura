// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TargetDataUnderMouse.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "TargetDataUnderMagicCircle.generated.h"

UCLASS()
class AURA_API UTargetDataUnderMagicCircle : public UAbilityTask
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (DisplayName = "TargetDataUnderMagicCircle", HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UTargetDataUnderMagicCircle* CreateTargetDataUnderMagicCircle(UGameplayAbility* OwningAbility);
	
	UPROPERTY(BlueprintAssignable)
	FMouseTargetDataSignature ValidData;

private:
	virtual void Activate() override;
	void SendMagicCircleData();

	void OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& DataHandle, FGameplayTag ActivationTag);
};
