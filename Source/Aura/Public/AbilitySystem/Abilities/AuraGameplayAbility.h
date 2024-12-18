// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "AuraGameplayAbility.generated.h"

UCLASS()
class AURA_API UAuraGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	// ���� �����Ƽ �� ��� �ο��� �� �� ���� Ȯ���ϱ�
	UPROPERTY(EditDefaultsOnly, Category="Input")
	FGameplayTag StartupInputTag;

	// Ŀ�� ���̺�
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category="Damage")
	FScalableFloat Damage;
};
