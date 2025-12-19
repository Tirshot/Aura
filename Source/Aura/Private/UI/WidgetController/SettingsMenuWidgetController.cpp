// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SettingsMenuWidgetController.h"
#include "Game/AuraGameUserSettings.h"

void USettingsMenuWidgetController::Initialize()
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		LastResolution = Settings->GetScreenResolution();
		LastWindowMode = Settings->GetFullscreenMode();
		LastTextureQuality = Settings->GetTextureQuality();
		LastAntiAliasingQuality = Settings->GetAntiAliasingQuality();
		LastShadowQuality = Settings->GetShadowQuality();
	}
}

FString USettingsMenuWidgetController::IntPointToString(FIntPoint IntPoint)
{
	FString X = FString::FromInt(IntPoint.X);
	FString Y = FString::FromInt(IntPoint.Y);

	FString WidthAppendMultiply = X.Append(TEXT(" x "));
	return WidthAppendMultiply.Append(Y);
}

FIntPoint USettingsMenuWidgetController::StringToIntPoint(FString Str)
{
	// x 기준으로 자르기
	TArray<FString> Strs;
	Str.ParseIntoArray(Strs, TEXT("x"), true);

	if (Strs.Num() != 2)
		return UAuraGameUserSettings::GetAuraGameUserSettings()->GetScreenResolution();
	
	// 앞 뒤의 공백 제거
	FString WidthString = Strs[0].TrimStartAndEnd();
	int32 Width = FCString::Atoi(*WidthString);

	FString HeightString = Strs[1].TrimStartAndEnd();
	int32 Height = FCString::Atoi(*HeightString);

	return FIntPoint(Width, Height);
}

EWindowMode::Type USettingsMenuWidgetController::ConvertIntToWindowModeType(int32 Index)
{
	return EWindowMode::ConvertIntToWindowMode(Index);
}

FString USettingsMenuWidgetController::GetDefaultResolution()
{
	FIntPoint DefaultResolution = UAuraGameUserSettings::GetAuraGameUserSettings()->GetScreenResolution();
	if (DefaultResolution.Size() == 0)
		return TEXT("");
	
	FString Str = IntPointToString(DefaultResolution);
	return Str;
}

EWindowMode::Type USettingsMenuWidgetController::GetFullScreenMode()
{
	return UAuraGameUserSettings::GetAuraGameUserSettings()->GetFullscreenMode();
}

void USettingsMenuWidgetController::SetScreenResolution(FIntPoint IntPoint)
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		Settings->SetScreenResolution(IntPoint);
	}
}

void USettingsMenuWidgetController::SetFullscreenMode(EWindowMode::Type InType)
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		Settings->SetFullscreenMode(InType);
	}
}

void USettingsMenuWidgetController::ApplyAllDisplaySettings()
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		FIntPoint NewResolution = Settings->GetScreenResolution();
		EWindowMode::Type NewWindowModeType = Settings->GetFullscreenMode();

		// 설정 확인 팝업
		OnAreYouSurePopUp.Broadcast();
		
		// 팝업에서 유지 눌렀을때만 Save
		Settings->ApplyResolutionSettings(false);

		// 전체 화면일 경우에는 윈도우 위치를 변경하지 않음
		if (NewWindowModeType == EWindowMode::Fullscreen)
			return;
		
		if (TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow())
		{
			if (Window.IsValid())
			{
				Window->Resize(FVector2D(NewResolution.X, NewResolution.Y));

				// 윈도우 센터 조정
				FIntPoint ResIntPoint = Settings->GetDesktopResolution();
				FVector2D Resolution(ResIntPoint.X, ResIntPoint.Y);

				FVector2D NewPosition = (Resolution - NewResolution) / 2.f;
				Window->MoveWindowTo(NewPosition);
			}
		}
	}
}

void USettingsMenuWidgetController::CheckSettingsChanged(FIntPoint NewResolution, EWindowMode::Type NewType)
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		// 지금 적용중인 해상도 및 설정들 확인
		FIntPoint CurrentResolution = Settings->GetScreenResolution();
		EWindowMode::Type CurrentWindowModeType = GetFullScreenMode();
		
		// 새로 적용하는 해상도 및 설정들과 비교
		if (CurrentResolution == NewResolution && CurrentWindowModeType == NewType)
		{
			// 버튼 비활성화 델리게이트 호출
			OnApplyButtonActivationChanged.Broadcast(false);
		}

		// 버튼 활성화 델리게이트 호출
		OnApplyButtonActivationChanged.Broadcast(true);
	}
	UE_LOG(LogTemp, Warning, TEXT("GameUserSettings Not Found!!"));
	OnApplyButtonActivationChanged.Broadcast(true);
}

void USettingsMenuWidgetController::OnPopUpConfirmClicked()
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		Settings->ApplySettings(false);
		LastResolution = Settings->GetScreenResolution();
		LastWindowMode = Settings->GetFullscreenMode();
		LastTextureQuality = Settings->GetTextureQuality();
		LastAntiAliasingQuality = Settings->GetAntiAliasingQuality();
		LastShadowQuality = Settings->GetShadowQuality();
		Settings->SaveSettings();
	}
}

void USettingsMenuWidgetController::OnPopUpCancelClicked()
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		Settings->SetScreenResolution(LastResolution);
		Settings->SetFullscreenMode(LastWindowMode);
		Settings->ApplyResolutionSettings(false);
		
		Settings->SetTextureQuality(LastTextureQuality);
		Settings->SetAntiAliasingQuality(LastAntiAliasingQuality);
		Settings->SetShadowQuality(LastShadowQuality);
		Settings->ApplySettings(false);
	}
}

void USettingsMenuWidgetController::SetTextureQuality(int32 Value)
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		Settings->SetTextureQuality(Value);
	}
}

void USettingsMenuWidgetController::SetAntiAliasingQuality(int32 Value)
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		Settings->SetAntiAliasingQuality(Value);
	}
}

void USettingsMenuWidgetController::SetShadowQuality(int32 Value)
{
	if (UAuraGameUserSettings* Settings = UAuraGameUserSettings::GetAuraGameUserSettings())
	{
		Settings->SetShadowQuality(Value);
	}
}
