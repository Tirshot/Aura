// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Interaction/CombatInterface.h"
#include "AuraPlayerState.generated.h"

class UCharmComponent;
class UEquipmentComponent;
class UInventoryComponent;
struct FAuraAbilityUpgradeInfo;
class UAttributeSet;
class ULevelUpInfo;

USTRUCT(BlueprintType)
struct FOwnedAbilityUpgrade : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
public:
	FOwnedAbilityUpgrade() {UpgradeTag = FGameplayTag::EmptyTag; UpgradeStack = 0;}
	FOwnedAbilityUpgrade(FGameplayTag InTag, int32 InStack) : UpgradeTag(InTag), UpgradeStack(InStack) {}
	FOwnedAbilityUpgrade(FGameplayTag InTag) : UpgradeTag(InTag), UpgradeStack(1) {}
	
	UPROPERTY()
	FGameplayTag UpgradeTag = FGameplayTag::EmptyTag;
	
	UPROPERTY()
	int32 UpgradeStack = 0;
		
	void PostReplicatedAdd(const struct FOwnedAbilityUpgradeList& InArraySerializer);
	void PostReplicatedChange(const struct FOwnedAbilityUpgradeList& InArraySerializer);
	void PreReplicatedRemove(const struct FOwnedAbilityUpgradeList& InArraySerializer);
};

USTRUCT(BlueprintType)
struct FOwnedAbilityUpgradeList : public FFastArraySerializer
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<FOwnedAbilityUpgrade> OwnedAbilityUpgrades;
	
public:
	bool FindOwnedAbilityUpgrade(const FGameplayTag& InTag);
	FOwnedAbilityUpgrade* GetOwnedAbilityUpgrade(const FGameplayTag& InTag);
	
	// 직렬화
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FastArrayDeltaSerialize<FOwnedAbilityUpgrade, FOwnedAbilityUpgradeList>(OwnedAbilityUpgrades, DeltaParms, *this);
	}
};

// FastArray 등록
template<>
struct TStructOpsTypeTraits<FOwnedAbilityUpgradeList> : public TStructOpsTypeTraitsBase2<FOwnedAbilityUpgradeList>
{
	enum { WithNetDeltaSerializer = true };
};

// 플레이어 스테이트 초기화 완료
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStateInitialized, AAuraPlayerState*, InitializedPlayerState);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStatChanged, int32, Value);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnLevelChanged, int32/*Level*/, bool /*bLevelUp*/);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCardsInitialized, TArray<FAuraAbilityUpgradeInfo>&, InitializedCards);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAbilityUpgradeTagsChanged, FGameplayTag, UpgradeTag, int32, Stack);

UCLASS()
class AURA_API AAuraPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AAuraPlayerState();

protected:
	virtual void BeginPlay() override;
	virtual void CopyProperties(APlayerState* PlayerState) override;
	
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }
	FOnDeath* GetOnDeathDelegate() {return &OnDeath;}
	FOnDamageSignature& GetOnDamageDelegate() {return OnDamageDelegate;}
	UInventoryComponent* GetInventoryComponent() {return Inventory;}
	UEquipmentComponent* GetEquipmentComponent() {return Equipment;}
	UCharmComponent* GetCharmComponent() {return Charm;}
	
public:
	// 레벨 정보
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<ULevelUpInfo> LevelUpInfo;

	UPROPERTY()
	FOnPlayerStateInitialized OnPlayerStateInitialized;

	// 값 변화 델리게이트
	FOnPlayerStatChanged OnXPChangedDelegate;
	FOnLevelChanged OnLevelChangedDelegate;

	// 라이프 사이클, 데미지
	FOnDeath OnDeath;
	FOnDamageSignature OnDamageDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnPlayerStatChanged OnAttributePointChangedDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnPlayerStatChanged OnSpellPointChangedDelegate;

	UPROPERTY()
	FOnCardsInitialized OnUpgradeCardsInitializedDelegate;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnAbilityUpgradeTagsChanged OnAbilityUpgradeTagsChangedDelegate;
	
public:
	FORCEINLINE int32 GetCharacterLevel() const { return Level; }
	FORCEINLINE int32 GetXP() const { return XP; }
	
	UFUNCTION(BlueprintCallable)
	FORCEINLINE int32 GetAttributePoints() const { return AttributePoints; }

	UFUNCTION(BlueprintCallable)
	FORCEINLINE int32 GetSpellPoints() const { return SpellPoints; }
	
	void SetXP(int32 GainedXP);
	void AddToXP(int32 GainedXP);
	void SetLevel(int32 InLevel);

	UFUNCTION(BlueprintCallable)
	void AddToLevelOne();
	void AddToLevel(int32 InLevel);
	
	void SetAttributePoints(int32 InAP);
	void AddToAttributePoints(int32 InAP);
	void SetSpellPoints(int32 InSP);
	void AddToSpellPoints(int32 InSP);

	void SetHealth(const float InHealth);
	void SetMana(const float InMana);

	void SetUpgradeCardInfo(const TArray<FAuraAbilityUpgradeInfo>& NewCard);

	FOwnedAbilityUpgradeList& GetOwnedAbilityUpgradeList(){ return OwnedAbilityUpgradeList; }
	void SetAbilityUpgradeTagContainer(const FOwnedAbilityUpgradeList& InOwnedAbilityUpgradeList);
	
	void ResetAttributesToBaseValue();
	
	void InitializeDefaultAttributesFromAttributeSet(UAbilitySystemComponent* NewASC, UAttributeSet* AS);
	
	void UpdateAbilityStatus();
	
public:
	// 어빌리티 업그레이드
	UPROPERTY(Replicated)
	FOwnedAbilityUpgradeList OwnedAbilityUpgradeList;
	
	UFUNCTION(Server, Reliable)
	void Server_AddAbilityUpgradeTag(FGameplayTag UpgradeTag);
	
	UFUNCTION(Server, Reliable)
	void Server_RemoveAbilityUpgradeTag(FGameplayTag UpgradeTag);

	void AddUpgradeTag(const FGameplayTag& Tag);
	void RemoveUpgradeTag(const FGameplayTag& Tag);

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

	UFUNCTION()
	void HandleAbilitiesSet();
	
	UFUNCTION()
	void OnASCRegistered(UAbilitySystemComponent* ASC);
	
protected:
	// GAS
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;
	
public:
	// 세이브, 로드
	UPROPERTY()
	bool bIsDataLoaded = false;
	
	bool bIsLevelInitialized = false;
	
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

	// UFUNCTION()
	// void OnRep_AbilityUpgradeTags();

	UFUNCTION()
	void OnRep_UpgradeCardInfo();
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess = true))
	TObjectPtr<class UInventoryComponent> Inventory;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess = true))
	TObjectPtr<class UEquipmentComponent> Equipment;
	
public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess = true))
	TObjectPtr<class UCharmComponent> Charm;
};
