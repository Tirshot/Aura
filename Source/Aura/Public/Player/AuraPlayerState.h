// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "AuraPlayerState.generated.h"

struct FAuraAbilityUpgradeInfo;
class UAttributeSet;
class ULevelUpInfo;

// 플레이어 스테이트 초기화 완료
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateInitialized, AAuraPlayerState*, InitializedPlayerState);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerStatChanged, int32);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLevelChanged, int32/*Level*/, bool /*bLevelUp*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardsInitialized, TArray<FAuraAbilityUpgradeInfo>&, InitializedCards);

UCLASS()
class AURA_API AAuraPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AAuraPlayerState();

protected:
	virtual void BeginPlay() override;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	// 레벨 정보
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<ULevelUpInfo> LevelUpInfo;

	UPROPERTY()
	FOnPlayerStateInitialized OnPlayerStateInitialized;
	
	// 값 변화 델리게이트
	FOnPlayerStatChanged OnXPChangedDelegate;
	FOnLevelChanged OnLevelChangedDelegate;
	FOnPlayerStatChanged OnAttributePointChangedDelegate;
	FOnPlayerStatChanged OnSpellPointChangedDelegate;

	UPROPERTY()
	FOnCardsInitialized OnUpgradeCardsInitializedDelegate;
	
	FORCEINLINE int32 GetCharacterLevel() const { return Level; }
	FORCEINLINE int32 GetXP() const { return XP; }
	FORCEINLINE int32 GetAttributePoints() const { return AttributePoints; }
	FORCEINLINE int32 GetSpellPoints() const { return SpellPoints; }
	void SetXP(int32 GainedXP);
	void AddToXP(int32 GainedXP);
	void SetLevel(int32 InLevel);
	void AddToLevel(int32 InLevel);
	void SetAttributePoints(int32 InAP);
	void AddToAttributePoints(int32 InAP);
	void SetSpellPoints(int32 InSP);
	void AddToSpellPoints(int32 InSP);

	void SetHealth(const float InHealth);
	void SetMana(const float InMana);

	void SetUpgradeCardInfo(const TArray<FAuraAbilityUpgradeInfo>& NewCard);

	FGameplayTagContainer& GetAbilityUpgradeTagContainer(){ return OwnedAbilityUpgradeTags; }
	void SetAbilityUpgradeTagContainer(const FGameplayTagContainer& InTagContainer);
	
public:
	// 어빌리티 업그레이드
	UPROPERTY(ReplicatedUsing = OnRep_AbilityUpgradeTags)
	FGameplayTagContainer OwnedAbilityUpgradeTags;

	UFUNCTION(Server, Reliable)
	void Server_AddAbilityUpgradeTag(FGameplayTag UpgradeTag);
	
	UFUNCTION(Server, Reliable)
	void Server_RemoveAbilityUpgradeTag(FGameplayTag UpgradeTag);

	UPROPERTY(ReplicatedUsing = OnRep_UpgradeCardInfo)
	TArray<FAuraAbilityUpgradeInfo> ReplicatedCardInfo;

public:
	UFUNCTION(BlueprintCallable)
	int32 GetUpgradeTagCount(FGameplayTag UpgradeTag);

	UFUNCTION()
	bool HasUpgradeTag(FGameplayTag UpgradeTag);

	UFUNCTION()
	TArray<FGameplayTag> GetAllAbilityTags();

	// 활성화 모든 어빌리티의 태그를 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GAS|Abilities")
	TArray<FGameplayTag> GetAllActiveAbilityTags() const;

	// 활성화 되지 않은 모든 어빌리티의 태그를 반환
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "GAS|Abilities")
	TArray<FGameplayTag> GetAllInActiveAbilityTags() const;

	void GetRandomAttributeUpgrade();

	// // 활성화된 어빌리티 중 3개 선택
	// TArray<FGameplayTag> GetRandomActivatedAbilityTags_Three(const FGameplayTagContainer& ActivatedAbilityTags);
	//
	// // 선택된 어빌리티 중 
	// TArray<FGameplayTag> GetRandomUpgradeTagsForActivatedAbility_Three();
	//
	// TArray<FAuraAbilityUpgradeInfo> GetRandomUpgradeInfosForActivatedAbility_Three();
	//
	UFUNCTION()
	void HandleAbilitiesSet();
	
protected:
	// GAS
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

private:
	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Level)
	int32 Level = 1;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_XP)
	int32 XP = 0;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_AttributePoint)
	int32 AttributePoints = 0;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_SpellPoint)
	int32 SpellPoints = 0;

	UFUNCTION()
	void OnRep_Level(int32 OldLevel);

	UFUNCTION()
	void OnRep_XP(int32 OldXP);

	UFUNCTION()
	void OnRep_AttributePoint(int32 OldAttributePoint);

	UFUNCTION()
	void OnRep_SpellPoint(int32 OldSpellPoint);

	UFUNCTION()
	void OnRep_AbilityUpgradeTags();

	UFUNCTION()
	void OnRep_UpgradeCardInfo();
};
