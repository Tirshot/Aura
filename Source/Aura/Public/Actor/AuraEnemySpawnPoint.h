// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MissionActor.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "Engine/TargetPoint.h"
#include "AuraEnemySpawnPoint.generated.h"

class AAuraEnemy;

UCLASS()
class AURA_API AAuraEnemySpawnPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void SpawnEnemy();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Class")
	TSubclassOf<AAuraEnemy> EnemyClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Class")
	int32 EnemyLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Class")
	ECharacterClass CharacterClass = ECharacterClass::Warrior;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Mission")
	TArray<TObjectPtr<AMissionActor>> MissionActors;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Class")
	bool bXPOverride = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Class", meta = (EditCondition = bXPOverride, EditConditionHides))
	float XPOverrideValue = 0.f;
	
};
