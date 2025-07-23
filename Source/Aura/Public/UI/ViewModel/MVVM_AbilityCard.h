// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "MVVM_AbilityCard.generated.h"

struct FAuraAbilityUpgradeInfo;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeSelected, FGameplayTag, SelectedUpgradeTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeInfoSelected, TArray<FAuraAbilityUpgradeInfo>&, SelectedUpgradeInfos);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeTagAssigned, const FGameplayTag&, UpgradeTag);

struct FGameplayTag;

UCLASS()
class AURA_API UMVVM_AbilityCard : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 CardIndex;

public:
	FOnUpgradeSelected OnUpgradeSelectedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnUpgradeTagAssigned OnUpgradeTagAssignedDelegate;

	FOnUpgradeInfoSelected OnUpgradeInfoSelectedDelegate;
	
public:
	FGameplayTag GetUpgradeTag() const { return UpgradeTag; }
	FString GetUpgradeName() const { return UpgradeName; }
	FText GetUpgradeDescription() const { return UpgradeDescription; }
	int32 GetUpgradeMaxLevel() const { return UpgradeMaxLevel; }

	void SetUpgradeTag(FGameplayTag InGameplayTag);
	void SetUpgradeName(FString InUpgradeName);
	void SetUpgradeDescription(FText InUpgradeDescription);
	void SetUpgradeMaxLevel(int32 InUpgradeMaxLevel);

	UFUNCTION(BlueprintCallable)
	void UpgradeButtonClicked();

private:
	/*필드 노티파이*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	FGameplayTag UpgradeTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	FString UpgradeName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	FText UpgradeDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	int32 UpgradeMaxLevel;
};
