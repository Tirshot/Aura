// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "AuraGameInstance.generated.h"

UCLASS()
class AURA_API UAuraGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	// 저장 슬롯
	UPROPERTY()
	FName PlayerStartTag = FName();
	
	UPROPERTY(BlueprintReadOnly)
	FString LoadSlotName = FString();

	UPROPERTY()
	int32 LoadSlotIndex = 0;

public:
	// 디버그 옵션 Setter/Getter
	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	bool IsVisibleNextButton() const { return bVisibleNextButton; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	void SetVisibleNextButton(bool bVisible) { bVisibleNextButton = bVisible; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	bool IsVisibleLevelUpButton() const { return bVisibleLevelUpButton; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	void SetVisibleLevelUpButton(bool bVisible) { bVisibleLevelUpButton = bVisible; }
	
	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	bool IsAuraInvincible() const { return bAuraInvincible; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	void SetAuraInvincible(bool bInvincible) { bAuraInvincible = bInvincible; }
	
	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	bool IsAuraInfiniteMana() const { return bAuraInfiniteMana; }

	UFUNCTION(BlueprintCallable, Category = "Aura Debug Settings")
	void SetAuraInfiniteMana(bool bInfiniteMana) { bAuraInfiniteMana = bInfiniteMana; }

	void SetAllVariablesToDefault();
	
protected:
	// 디버그 옵션 변수
	UPROPERTY()
	bool bVisibleNextButton = false;
	
	UPROPERTY()
	bool bVisibleLevelUpButton = false;
	
	UPROPERTY()
	bool bAuraInvincible = false;
	
	UPROPERTY()
	bool bAuraInfiniteMana = false;
};
