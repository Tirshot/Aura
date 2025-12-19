// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameUserSettings.h"

UAuraGameUserSettings* UAuraGameUserSettings::GetAuraGameUserSettings()
{
	return Cast<UAuraGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}