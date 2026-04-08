// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/SoundData.h"
#include "Subsystems/WorldSubsystem.h"
#include "AuraAudioSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UAuraAudioSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void OnWorldBeginPlay(UWorld& InWorld) override;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Aura|AudioSubSystem")
	void PlayMusic(USoundBase* Sound, float StartTime, bool bLooping, float LoopStart, float LoopDuration, FGameplayTag SoundTag = FGameplayTag());
	
	UFUNCTION(BlueprintCallable, Category = "Aura|AudioSubSystem")
	void PlayMusicByTag(FGameplayTag SoundTag, float StartTime, bool bLooping, float LoopStart, float LoopDuration);
	
	UFUNCTION(BlueprintCallable, Category = "Aura|AudioSubSystem")
	void PlayMusicByTag_NoVariable(FGameplayTag SoundTag);
	
	UFUNCTION(BlueprintCallable, Category = "Aura|AudioSubSystem")
	void StopMusic();
	
	UFUNCTION(BlueprintCallable, Category = "Aura|AudioSubSystem")
	void UpdateSoundVolume(USoundClass* TargetSoundClass, float Volume);
	
private:
	// 재생중인 BGM 사운드 컴포넌트
	UPROPERTY()
	TObjectPtr<UAudioComponent> SoundComponent;
	
	UPROPERTY()
	TObjectPtr<USoundData> LoadedSoundData;
	
	UPROPERTY()
	TObjectPtr<USoundMix> LoadedSoundMix;
	
	UPROPERTY()
	FGameplayTag CurrentPlayingSoundTag; 
};
