// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "SettingsMenuWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnApplyButtonActivationChanged, bool, bActivate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAreYouSurePopUp);

UCLASS(BlueprintType, Blueprintable)
class AURA_API USettingsMenuWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void Initialize();

public:
	// 디스플레이 설정
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnApplyButtonActivationChanged OnApplyButtonActivationChanged;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnAreYouSurePopUp OnAreYouSurePopUp;
	
	UFUNCTION(BlueprintCallable)
	FString IntPointToString(FIntPoint IntPoint);
	
	UFUNCTION(BlueprintCallable)
	FIntPoint StringToIntPoint(FString Str);

	UFUNCTION(BlueprintCallable)
	EWindowMode::Type ConvertIntToWindowModeType(int32 Index);

	UFUNCTION(BlueprintCallable)
	FString GetDefaultResolution();
	
	UFUNCTION(BlueprintCallable)
	EWindowMode::Type GetFullScreenMode();

	UFUNCTION(BlueprintCallable)
	void SetScreenResolution(FIntPoint IntPoint);

	UFUNCTION(BlueprintCallable)
	void SetFullscreenMode(EWindowMode::Type InType);
	
	UFUNCTION(BlueprintCallable)
	void ApplyAllDisplaySettings();
	
	UFUNCTION(BlueprintCallable)
	void CheckSettingsChanged(FIntPoint NewResolution, EWindowMode::Type NewType);


public:
	// 팝업 메뉴 콜백 함수
	UFUNCTION(BlueprintCallable)
	void OnPopUpConfirmClicked();
	
	UFUNCTION(BlueprintCallable)
	void OnPopUpCancelClicked();

public:
	// 그래픽 설정
	UFUNCTION(BlueprintCallable)
	void SetTextureQuality(int32 Value);
	
	UFUNCTION(BlueprintCallable)
	void SetAntiAliasingQuality(int32 Value);
	
	UFUNCTION(BlueprintCallable)
	void SetShadowQuality(int32 Value);
	
public:
	// 사운드 설정
	UFUNCTION(BlueprintCallable)
	void SetMasterVolume(USoundClass* SoundClass, float Value);
	
	UFUNCTION(BlueprintCallable)
	void SetBackgroundVolume(USoundClass* SoundClass, float Value);
	
	UFUNCTION(BlueprintCallable)
	void SetFXVolume(USoundClass* SoundClass, float Value);

	UFUNCTION(BlueprintCallable)
	void SetUIVolume(USoundClass* SoundClass, float Value);	
	
	// 사운드 설정
	UFUNCTION(BlueprintCallable)
	void SetMasterVolumeChecked(USoundClass* SoundClass, bool Value);
	
	UFUNCTION(BlueprintCallable)
	void SetBackgroundVolumeChecked(USoundClass* SoundClass, bool Value);
	
	UFUNCTION(BlueprintCallable)
	void SetFXVolumeChecked(USoundClass* SoundClass, bool Value);

	UFUNCTION(BlueprintCallable)
	void SetUIVolumeChecked(USoundClass* SoundClass, bool Value);

public:
	UFUNCTION(BlueprintCallable)
	EWindowMode::Type GetLastWindowMode(){return LastWindowMode;}
	
	UFUNCTION(BlueprintCallable)
	FIntPoint GetLastResolution(){return LastResolution;}

	UFUNCTION(BlueprintCallable)
	int32 GetLastTextureQuality(){return LastTextureQuality;}
	
	UFUNCTION(BlueprintCallable)
	int32 GetLastAntiAliasingQuality(){return LastAntiAliasingQuality;}
	
	UFUNCTION(BlueprintCallable)
	int32 GetLastShadowQuality(){return LastShadowQuality;}
	
public:
	// 그래픽 설정 멤버 변수
	FIntPoint LastResolution;
	EWindowMode::Type LastWindowMode;
	int32 LastTextureQuality;
	int32 LastAntiAliasingQuality;
	int32 LastShadowQuality;
	
	// 사운드 설정 멤버 변수
	UPROPERTY(BlueprintReadOnly)
	float LastMasterVolume;
	UPROPERTY(BlueprintReadOnly)
	float LastUIVolume;
	UPROPERTY(BlueprintReadOnly)
	float LastFXVolume;
	UPROPERTY(BlueprintReadOnly)
	float LastBackgroundVolume;
	
	UPROPERTY(BlueprintReadOnly)
	bool LastMasterVolumeChecked;
	UPROPERTY(BlueprintReadOnly)
	bool LastUIVolumeChecked;
	UPROPERTY(BlueprintReadOnly)
	bool LastFXVolumeChecked;
	UPROPERTY(BlueprintReadOnly)
	bool LastBackgroundVolumeChecked;
};
