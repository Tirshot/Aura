
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Interaction/CombatInterface.h"
#include "AuraCharacterBase.generated.h"

struct FOnAttributeChangeData;
struct FGameplayEffectSpecHandle;
class UAbilitySystemComponent;
class UAttributeSet;
class UGameplayEffect;
class UGameplayAbility;
class UAnimMontage;
class UNiagaraSystem;
class UDebuffNiagaraComponent;
class UPassiveNiagaraComponent;

UCLASS(Abstract)
class AURA_API AAuraCharacterBase : public ACharacter, public IAbilitySystemInterface, public ICombatInterface
{
	GENERATED_BODY()

public:
	AAuraCharacterBase();

	virtual void Tick(float DeltaTime) override;
	virtual void PossessedBy(AController* NewController) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const;
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }
	UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** Combat Interface 시작 **/
	virtual UAnimMontage* GetHitReactMontage_Implementation() override;
	virtual void Die(const FVector& DeathImpulse, AAuraCharacter* KilledBy) override;
	virtual FVector GetCombatSocketLocation_Implementation(const FGameplayTag& MontageTag) override;
	virtual bool IsDead_Implementation() const override;
	virtual FOnDeath* GetOnDeathDelegate() override;
	virtual AActor* GetAvatar_Implementation() override;
	virtual TArray<FTaggedMontage> GetAttackMontages_Implementation() override;
	virtual UNiagaraSystem* GetBloodEffect_Implementation() override;
	virtual FTaggedMontage GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag) override;
	virtual int32 GetMinionCount_Implementation() override;
	virtual void IncrementMinionCount_Implementation(int32 Amount) override;
	virtual ECharacterClass GetCharacterClass_Implementation() override;
	virtual FOnASCRegistered& GetOnASCRegisteredDelegate() override;
	virtual USkeletalMeshComponent* GetWeapon_Implementation() override;
	virtual bool IsBeingShocked_Implementation() override;
	virtual void SetIsBeingShocked_Implementation(bool bInShock) override;
	virtual UAttributeSet* GetAttributeSet_Implementation() override;
	virtual FOnDamageSignature& GetOnDamageDelegate() override;
	virtual void ShowRangeIndicator_Implementation(bool bAttachToActor, ERangeShape RangeShape, const FVector& Location, float Radius, float Width, float Height, const FVector& RGB) override;
	virtual void HideRangeIndicator_Implementation() const override;
	virtual void SetCharacterInvincible_Implementation(bool InbInvincible) override;
	virtual bool IsCharacterInvincible_Implementation() const override;
	virtual void SetCharacterInfiniteMana_Implementation(bool InbInfinite) override;
	virtual bool IsCharacterInfiniteMana_Implementation() const override;
	virtual void SetCharacterDebugInvincible_Implementation(bool InbInvincible) override;
	/** Combat Interface 끝 **/

	FOnASCRegistered OnASCRegistered;
	void ASCRegistered(UAbilitySystemComponent* ASC);
	
	UFUNCTION()
	void AuraPlayerStateInitialized(AAuraPlayerState* AuraPS);
	void OnMovementSpeedChanged(const FOnAttributeChangeData& Data);
	
	UFUNCTION(BlueprintCallable)
	void StopMovementInput();
	
	UFUNCTION(BlueprintCallable)
	void RemoveAllMinions();

	// RPC
	UFUNCTION(NetMulticast, Reliable)
	virtual void MulticastHandleDeath(const FVector& DeathImpulse);
	
	UPROPERTY(EditAnywhere, category="Combat")
	TArray<FTaggedMontage> AttackMontages;

	// 상태 이상
	UPROPERTY(ReplicatedUsing=OnRep_Stunned, BlueprintReadOnly)
	bool bIsStunned = false;

	UPROPERTY(ReplicatedUsing=OnRep_Burned, BlueprintReadOnly)
	bool bIsBurned = false;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsBeingShock = false;

	virtual void StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	virtual void StackStunTagChanged(const FGameplayEffectSpecHandle SpecHandle, int32 NewStack, int32 OldStack);
	virtual void BeingShockedTagChanged();

	void SetCharacterClass(ECharacterClass InClass) {CharacterClass = InClass;}
	
	UFUNCTION()
	virtual void OnRep_Stunned();

	UFUNCTION()
	virtual void OnRep_Burned();
	
	UFUNCTION()
	virtual void OnRep_Invincible();
	
protected:
	virtual void BeginPlay() override;
	virtual void InitAbilityActorInfo();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	TObjectPtr<USkeletalMeshComponent> Weapon;

	// 무기 소켓
	UPROPERTY(EditAnywhere, Category = "Combat")
	FName WeaponTipSocketName;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FName LeftHandSocketName;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FName RightHandSocketName;

	UPROPERTY(EditAnywhere, Category = "Combat")
	FName TailSocketName;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bDead = false;
		
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing=OnRep_Invincible)
	bool bInvincible = false;
		
	// GAS
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

public:
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> DefaultSecondaryAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> InitializeVitalAttributes;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Attributes")
	TSubclassOf<UGameplayEffect> InitializeRegenAttributes;
	
	UPROPERTY(EditDefaultsOnly, Category = "Abilities | Effects")
	TSubclassOf<UGameplayEffect> InvincibleGameplayEffectClass;
		
	UPROPERTY(EditDefaultsOnly, Category = "Abilities | Effects")
	TSubclassOf<UGameplayEffect> DebugInvincibleGameplayEffectClass;
		
	UPROPERTY(EditDefaultsOnly, Category = "Abilities | Effects")
	TSubclassOf<UGameplayEffect> InfiniteManaGameplayEffectClass;
	
	// 소환수 사망시키는 이펙트
	UPROPERTY(EditDefaultsOnly, Category = "Abilities | Effects")
	TSubclassOf<UGameplayEffect> DeadEffectClass;
	
	void ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const;
	
protected:
	virtual void InitializeDefaultAttributes() const;

	// ������ ����Ʈ
	void Dissolve();

	UFUNCTION(BlueprintImplementableEvent)
	void StartDissolveTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);

	UFUNCTION(BlueprintImplementableEvent)
	void StartWeaponDissolveTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInstance> DissolveMaterialInstance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UMaterialInstance> WeaponDissolveMaterialInstance;

	// 혈흔
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UNiagaraSystem* BloodEffect;

	// 사망 사운드
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	USoundBase* DeathSound;

	// 미니언 소환 카운트
	int32 MinionCount = 0;

	// 미니언 포인터 배열
	UPROPERTY(BlueprintReadWrite)
	TArray<AActor*> Minions;

	// 미니언 자동 처치를 위한 데미지 이펙트 클래스
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DamageGameplayEffectClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, category = "Character Class Defaults")
	ECharacterClass CharacterClass = ECharacterClass::Warrior;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDebuffNiagaraComponent> BurnDebuffComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDebuffNiagaraComponent> StunDebuffComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDebuffNiagaraComponent> InvincibleBuffComponent;

protected:
	// 어빌리티 부여
	void AddCharacterAbilites();

private:
	// 시작 시 부여하는 액티브 어빌리티
	UPROPERTY(EditAnywhere, Category="Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	// 시작 시 부여하는 패시브 어빌리티
	UPROPERTY(EditAnywhere, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupPassiveAbilities;

	UPROPERTY(EditAnywhere, Category = "Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;

	// 패시브 스펠 이펙트
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPassiveNiagaraComponent> HaloOfProtectionNiagaraComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPassiveNiagaraComponent> LifeSiphonNiagaraComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPassiveNiagaraComponent> ManaSiphonNiagaraComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> EffectAttachComponent;
};
