// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AuraAbilitySystemComponent.generated.h"

class ULoadScreenSaveGame;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEffectAssetTags, const FGameplayTagContainer&, AssetTags/*에셋 태그들*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilitiesGiven);
DECLARE_DELEGATE_OneParam(FForEachAbility, const FGameplayAbilitySpec&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityStatusChanged, const FGameplayTag& ,AbilityTag, const FGameplayTag&, AbilityStatus, int32, AbilityLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FAbilityEquipped, const FGameplayTag&, AbilityTag, const FGameplayTag&, AbilityStatus, const FGameplayTag& /*슬롯*/, AbilitySlot, const FGameplayTag&, PrevSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDeactivatePassiveAbility, const FGameplayTag&, AbilityTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActivatePassiveEffect, const FGameplayTag&, AbilityTag, bool, bActivate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageTagReceived, const FGameplayTag&, CueTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageRemoved, const FGameplayTag&, Tag/*메시지 게임플레이 큐 태그*/);

UCLASS()
class AURA_API UAuraAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	// 델리게이트 바인딩
	void AbilityActorInfoSet();
	
	virtual void BeginDestroy() override;

	UPROPERTY()
	FEffectAssetTags EffectAssetTags;
	
	UPROPERTY()
	FAbilitiesGiven AbilitiesGivenDelegate;
	
	UPROPERTY()
	FAbilityStatusChanged AbilityStatusChanged;
	
	UPROPERTY()
	FAbilityEquipped AbilityEquipped;
	
	UPROPERTY()
	FDeactivatePassiveAbility DeactivePassiveAbility;
	
	UPROPERTY()
	FActivatePassiveEffect ActivatePassiveEffect;
	
	UPROPERTY()
	FMessageTagReceived OnMessageTagReceived;
	
	UPROPERTY()
	FMessageRemoved OnMessageRemoved;

	// 어빌리티 부여
	void AddCharacterAbilitiesFromSaveData(ULoadScreenSaveGame* SaveData);
	void AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupAbilities);
	void AddCharacterPassiveAbilities(const TArray<TSubclassOf<UGameplayAbility>>& StartupPassiveAbilities);
	bool bStartupAbilitiesGiven = false;
	void AddCharacterAbilityByTag(const FGameplayTag& AbilityTag);
	void RemoveCharacterAbilityByTag(const FGameplayTag& AbilityTag, const int32 RemoveCount);

	// 어빌리티 입력
	void AbilityInputTagPressed(const FGameplayTag& InputTag);
	void AbilityInputTagHeld(const FGameplayTag& InputTag);
	void AbilityInputTagReleased(const FGameplayTag& InputTag);

	// 활성화 가능한 어빌리티 순회
	void ForEachAbility(const FForEachAbility& Delegate);

	// 게임플레이 어빌리티 스펙에 대한 태그 가져옴
	static FGameplayTag GetAbilityTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	static FGameplayTag GetInputTagFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	static FGameplayTag GetStatusFromSpec(const FGameplayAbilitySpec& AbilitySpec);
	static FGameplayTag GetStatusFromSpec(FGameplayAbilitySpec* AbilitySpec);
	FGameplayTag GetStatusFromAbilityTag(const FGameplayTag& AbilityTag);
	FGameplayTag GetSlotFromAbilityTag(const FGameplayTag& AbilityTag);
	bool SlotIsEmpty(const FGameplayTag& Slot);

	FGameplayAbilitySpec* GetSpecFromAbilityTag(const FGameplayTag& AbilityTag);

	void UpgradeAttribute(const FGameplayTag& AttributeTag);

	UFUNCTION(Server, Reliable)
	void ServerUpgradeAttribute(const FGameplayTag& AttributeTag);

	void UpdateAbilityStatus(int32 Level);

	UFUNCTION(Server, Reliable)
	void ServerSpendSpellPoint(const FGameplayTag& AbilityTag, bool bNotSpendPoint = false);

	UFUNCTION(Server, Reliable)
	void Server_RefundSpellPoint(const FGameplayTag& AbilityTag, bool bNotRefundPoint = false);
	
	UFUNCTION(Server, Reliable)
	void ServerEquipAbility(const FGameplayTag& AbilityTag, const FGameplayTag& Slot);

	UFUNCTION(Client, Reliable)
	void ClientEquipAbility(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PrevSlot);

	bool GetDescriptionsByAbilityTag(const FGameplayTag& AbilityTag, FString& OutDescription, FString& OutNextLevelDescription);
	int32 GetAbilityLevelByTag(const FGameplayTag& AbilityTag);
	static void ClearSlot(FGameplayAbilitySpec* Spec);
	void ClearAbilitiesOfSlot(const FGameplayTag& Slot);
	static bool AbilityHasSlot(FGameplayAbilitySpec& Spec, const FGameplayTag& Slot);
	static bool AbilityHasAnySlot(FGameplayAbilitySpec& Spec);
	FGameplayAbilitySpec* GetSpecWithSlot(const FGameplayTag& Slot);
	bool IsPassiveAbility(const FGameplayAbilitySpec& Spec) const;
	static void AssignSlotToAbility(FGameplayAbilitySpec& Spec, const FGameplayTag& Slot);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastActivatePassiveEffect(const FGameplayTag& AbilityTag, bool bActivate);
	
	UFUNCTION(BlueprintCallable)
	void MessageRemove(const FGameplayTag& Tag);
	
protected:
	// 어빌리티가 클라이언트로 복제(Replicated)
	virtual void OnRep_ActivateAbilities() override;

public:
	UFUNCTION(Client, Reliable)
	// 이펙트 액터 적용 콜백 함수
	void ClientEffectApplied(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle);

	UFUNCTION(Client, Reliable)
	void ClientUpdateAbilityStatus(const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 AbilityLevel);

protected:
	UPROPERTY()
	bool bStartAbilitiesGiven = false;

};
