// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "AuraGameUserSettings.generated.h"

UCLASS()
class AURA_API UAuraGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="AuraGameUserSettings")
	static UAuraGameUserSettings* GetAuraGameUserSettings();
	
public:
	float GetMasterVolume() { return MasterVolume; }
	float GetUIVolume() { return UIVolume; }
	float GetFXVolume() { return FXVolume; }
	float GetBackgroundVolume() { return BackgroundVolume; }
	
	void SetMasterVolume(float InMasterVolume){ MasterVolume = InMasterVolume; SaveConfig();}
	void SetUIVolume(float InUIVolume){ UIVolume = InUIVolume; SaveConfig();}
	void SetFXVolume(float InFXVolume){ FXVolume = InFXVolume; SaveConfig();}
	void SetBackgroundVolume(float InBackgroundVolume){ BackgroundVolume = InBackgroundVolume; SaveConfig();}
	
	bool GetMasterVolumeChecked() { return MasterVolumeChecked; }
	bool GetUIVolumeChecked() { return UIVolumeChecked; }
	bool GetFXVolumeChecked() { return FXVolumeChecked; }
	bool GetBackgroundVolumeChecked() { return BackgroundVolumeChecked; }
	
	void SetMasterVolumeChecked(bool InMasterVolumeChecked);
	void SetUIVolumeChecked(bool InUIVolumeChecked){ UIVolumeChecked = InUIVolumeChecked; SaveConfig();}
	void SetFXVolumeChecked(bool InFXVolumeChecked){ FXVolumeChecked = InFXVolumeChecked; SaveConfig();}
	void SetBackgroundVolumeChecked(bool InBackgroundVolumeChecked){ BackgroundVolumeChecked = InBackgroundVolumeChecked; SaveConfig();}
	
protected:
	UPROPERTY(Config)
	float MasterVolume = 1.f;
	
	UPROPERTY(Config)
	float UIVolume = 1.f;
	
	UPROPERTY(Config)
	float FXVolume = 1.f;
	
	UPROPERTY(Config)
	float BackgroundVolume = 1.f;
	
	UPROPERTY(Config)
	bool MasterVolumeChecked;
	
	UPROPERTY(Config)
	bool UIVolumeChecked;
	
	UPROPERTY(Config)
	bool FXVolumeChecked;
	
	UPROPERTY(Config)
	bool BackgroundVolumeChecked;
};
