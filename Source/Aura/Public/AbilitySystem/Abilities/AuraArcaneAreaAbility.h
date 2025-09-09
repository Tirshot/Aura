// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AuraArcaneAreaAbility.generated.h"

class AAuraArcaneArea;

UCLASS()
class AURA_API UAuraArcaneAreaAbility : public UAuraDamageGameplayAbility
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable)
	void SpawnArcaneArea(const FVector& Location);

	UFUNCTION()
	void OnArcaneAreaDestroyed(AActor* DestroyedActor);

	UFUNCTION(BlueprintCallable)
	void ApplySlowEffect(AActor* TargetActor);

	UFUNCTION()
	void OnSlowStackChanged(FGameplayEffectSpecHandle SpecHandle, int32 NewStack, int32 OldStack);

protected:
	UPROPERTY(EditDefaultsOnly)
	float SlowDownRatio = 0.25f;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AAuraArcaneArea> ArcaneAreaClass;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AAuraArcaneArea> ArcaneArea;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> SlowDownEffectClass;

	
};
