// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "SoundData.generated.h"

USTRUCT(BlueprintType)
struct FSoundContext
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag SoundTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<USoundBase> SoundAsset = nullptr;
	
	// 노래 시작 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float StartTime = 0.f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bLooping = false;
	
	// 루핑이 True일때만 나타나는 루핑 옵션
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bLooping", EditConditionHides))
	float LoopStartTime = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "bLooping", EditConditionHides))
	float LoopDuration = -1.f;
};

UCLASS()
class AURA_API USoundData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	USoundBase* GetSoundByTag(FGameplayTag Tag);
	FSoundContext GetSoundContextByTag(FGameplayTag Tag);
	
protected:
	UPROPERTY(EditDefaultsOnly)
	TArray<FSoundContext> SoundContexts;
};
