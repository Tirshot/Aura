// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MVVM_DebugMenu.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCheckBoxChecked, bool, bChecked);

UCLASS()
class AURA_API UMVVM_DebugMenu : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	virtual UWorld* GetWorld() const override;
	
	UFUNCTION(BlueprintCallable)
	void ViewModelInitialized();

public:
	UFUNCTION(BlueprintCallable)
	void VisibleNextButton(bool bVisible);
	
	UFUNCTION(BlueprintCallable)
	void VisibleLevelUpButton(bool bVisible);

	UFUNCTION(BlueprintCallable)
	void ApplyDebugInvincibleToAura(bool bInvincible);

	UFUNCTION(BlueprintCallable)
	void ApplyInfiniteManaToAura(bool bInfiniteMana);

public:
	bool GetbVisibleNextButton() const {return bVisibleNextButton;}
	bool GetbVisibleLevelUpButton() const {return bVisibleLevelUpButton;}
	bool GetbAuraDebugInvincible() const {return bAuraDebugInvincible;}
	bool GetbAuraInfiniteMana() const {return bAuraInfiniteMana;}
	
	void SetbVisibleNextButton(bool bVisible);
	void SetbVisibleLevelUpButton(bool bVisible);
	void SetbAuraDebugInvincible(bool bInvincible);
	void SetbAuraInfiniteMana(bool bInfiniteMana);

	UFUNCTION(BlueprintCallable)
	void OnForcingSaveButtonPressed();
	
private:
	/*필드 노티파이*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	bool bVisibleNextButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	bool bVisibleLevelUpButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	bool bAuraDebugInvincible;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	bool bAuraInfiniteMana;
};
