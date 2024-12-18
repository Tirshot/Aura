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
	// 시작 어빌리티 일 경우 부여할 때 한 번만 확인하기
	UPROPERTY(EditDefaultsOnly, Category="Input")
	FGameplayTag StartupInputTag;

	// 커브 테이블
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, category="Damage")
	FScalableFloat Damage;
};
