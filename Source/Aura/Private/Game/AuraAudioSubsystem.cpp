// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraAudioSubsystem.h"

#include "AuraGameplayTags.h"
#include "Components/AudioComponent.h"
#include "Game/AuraGameInstance.h"
#include "Kismet/GameplayStatics.h"

void UAuraAudioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UWorld* World = GetWorld();
	if (!World)
		return;
	
	UAuraGameInstance* AuraGI = World->GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return;
	
	LoadedSoundData = AuraGI->SoundData;
	LoadedSoundMix = AuraGI->SoundMix;
}

void UAuraAudioSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	UAuraGameInstance* AuraGI = InWorld.GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return;
	
	LoadedSoundData = AuraGI->SoundData;
	LoadedSoundMix = AuraGI->SoundMix;
	
	// 기본 배경음 재생
	PlayMusicByTag(FGameplayTag::RequestGameplayTag("Sound.Background.Ambient"), 2.f, true, 2.f, 90.f);
}

void UAuraAudioSubsystem::PlayMusic(USoundBase* Sound, float StartTime, bool bLooping, float LoopStart, float LoopDuration, FGameplayTag SoundTag)
{
	if (!Sound)
		return;

	UWorld* World = GetWorld();
	if (!World)
		return;
	
	// 이미 재생 중인 음악이 있다면 페이드 아웃, 정지
	if (SoundComponent && SoundComponent->IsPlaying())
	{
		// 동일한 사운드면 재생하지 않음
		if (SoundComponent->GetSound() == Sound)
			return;
		
		// 지금 재생중인 음악과 동일한 태그를 가지면 재생하지 않음
		if (CurrentPlayingSoundTag.MatchesTagExact(SoundTag))
			return;
		
		StopMusic();
	}
		
	SoundComponent = UGameplayStatics::CreateSound2D(
		World,
		Sound,
		1.f,
		1.f,
		StartTime,
		nullptr,
		true);
		
	// 오디오 컴포넌트 재생성 및 재생
	SoundComponent->SetFloatParameter(FName("StartTime"), StartTime);
	SoundComponent->SetBoolParameter(FName("bLooping"), bLooping);
	SoundComponent->SetFloatParameter(FName("LoopStart"), LoopStart);
	SoundComponent->SetFloatParameter(FName("LoopDuration"), LoopDuration);

	// 페이드인
	SoundComponent->FadeIn(0.5f, 1.0f);
	
	if (SoundTag.IsValid())
		CurrentPlayingSoundTag = SoundTag;
}

void UAuraAudioSubsystem::PlayMusicByTag(FGameplayTag SoundTag, float StartTime, bool bLooping, float LoopStart,
	float LoopDuration)
{
	if (!LoadedSoundData)
		return;
	
	if (USoundBase* SoundBase = LoadedSoundData->GetSoundByTag(SoundTag))
	{
		PlayMusic(SoundBase, StartTime, bLooping, LoopStart, LoopDuration, SoundTag);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AuraAudioSubsystem::PlayMusicByTag - Sound Not Found by Given GameplayTag : %s"), *SoundTag.ToString());
		CurrentPlayingSoundTag = FGameplayTag::EmptyTag;
	}
}

void UAuraAudioSubsystem::PlayMusicByTag_NoVariable(FGameplayTag SoundTag)
{
	if (!LoadedSoundData)
		return;
	
	FSoundContext SoundContext = LoadedSoundData->GetSoundContextByTag(SoundTag);
	
	if (USoundBase* SoundBase = LoadedSoundData->GetSoundByTag(SoundTag))
	{
		PlayMusic(SoundBase, SoundContext.StartTime, SoundContext.bLooping, SoundContext.LoopStartTime, SoundContext.LoopDuration, SoundTag);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AuraAudioSubsystem::PlayMusicByTag - Sound Not Found by Given GameplayTag : %s"), *SoundTag.ToString());
		CurrentPlayingSoundTag = FGameplayTag::EmptyTag;
	}
}

void UAuraAudioSubsystem::StopMusic()
{
	if (SoundComponent && SoundComponent->IsPlaying())
	{
		SoundComponent->FadeOut(0.5f, 0.0f);
		CurrentPlayingSoundTag = FGameplayTag::EmptyTag;
	}
}

void UAuraAudioSubsystem::UpdateSoundVolume(USoundClass* TargetSoundClass, float Volume)
{
	if (!TargetSoundClass)
		return;
	
	if (!LoadedSoundData || !LoadedSoundMix)
	{
		UWorld* World = GetWorld();
		if (!World)
			return;
	
		UAuraGameInstance* AuraGI = World->GetGameInstance<UAuraGameInstance>();
		if (!AuraGI)
			return;
	
		LoadedSoundData = AuraGI->SoundData;
		LoadedSoundMix = AuraGI->SoundMix;
	}
	
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetSoundMixClassOverride(
			World,
			LoadedSoundMix,
			TargetSoundClass,
			Volume);
		
		// 변경한 볼륨 즉시 적용
		UGameplayStatics::PushSoundMixModifier(World, LoadedSoundMix);
	}
}