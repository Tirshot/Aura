// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_DebugMenu.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Game/AuraGameInstance.h"
#include "Game/AuraGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerController.h"
#include "UI/WidgetController/OverlayWidgetController.h"

void UMVVM_DebugMenu::ViewModelInitialized()
{
	// 초기 값 가져오기
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		SetbVisibleLevelUpButton(AuraGI->IsVisibleLevelUpButton());
		VisibleLevelUpButton(AuraGI->IsVisibleLevelUpButton());
		
		SetbVisibleNextButton(AuraGI->IsVisibleNextButton());
		VisibleNextButton(AuraGI->IsVisibleNextButton());
		
		SetbAuraDebugInvincible(AuraGI->IsAuraInvincible());
		ApplyDebugInvincibleToAura(AuraGI->IsAuraInvincible());
		
		SetbAuraInfiniteMana(AuraGI->IsAuraInfiniteMana());
		ApplyInfiniteManaToAura(AuraGI->IsAuraInfiniteMana());
	}
}

void UMVVM_DebugMenu::VisibleNextButton(bool bVisible)
{
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		AuraGI->SetVisibleNextButton(bVisible);
	}
	if (UOverlayWidgetController* OverlayWC = UAuraAbilitySystemLibrary::GetOverlayWidgetController(this))
	{
		OverlayWC->OnNextButtonVisibilityChanged.Broadcast(bVisible);
	}
}

void UMVVM_DebugMenu::VisibleLevelUpButton(bool bVisible)
{
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		AuraGI->SetVisibleLevelUpButton(bVisible);
	}
	if (UOverlayWidgetController* OverlayWC = UAuraAbilitySystemLibrary::GetOverlayWidgetController(this))
	{
		OverlayWC->OnLevelUpButtonVisibilityChanged.Broadcast(bVisible);
	}
}

void UMVVM_DebugMenu::ApplyDebugInvincibleToAura(bool bInvincible)
{
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		AuraGI->SetAuraInvincible(bInvincible);
	}
	if (AAuraPlayerController* AuraPC = GetWorld()->GetFirstPlayerController<AAuraPlayerController>())
	{
		AuraPC->Server_CharacterDebugInvincible(bInvincible);
	}
}

void UMVVM_DebugMenu::ApplyInfiniteManaToAura(bool bInfiniteMana)
{
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		AuraGI->SetAuraInfiniteMana(bInfiniteMana);
	}
	if (AAuraPlayerController* AuraPC = GetWorld()->GetFirstPlayerController<AAuraPlayerController>())
	{
		AuraPC->Server_CharacterInfiniteMana(bInfiniteMana);
	}
}

void UMVVM_DebugMenu::SetbVisibleNextButton(bool bVisible)
{
	UE_MVVM_SET_PROPERTY_VALUE(bVisibleNextButton, bVisible);
}

void UMVVM_DebugMenu::SetbVisibleLevelUpButton(bool bVisible)
{
	UE_MVVM_SET_PROPERTY_VALUE(bVisibleLevelUpButton, bVisible);
}

void UMVVM_DebugMenu::SetbAuraDebugInvincible(bool bInvincible)
{
	UE_MVVM_SET_PROPERTY_VALUE(bAuraDebugInvincible, bInvincible);
}

void UMVVM_DebugMenu::SetbAuraInfiniteMana(bool bInfiniteMana)
{
	UE_MVVM_SET_PROPERTY_VALUE(bAuraInfiniteMana, bInfiniteMana);
}

void UMVVM_DebugMenu::OnForcingSaveButtonPressed()
{
	if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
	{
		AuraGM->GameAutoSave();
	}
}
