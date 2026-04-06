#include "Character/AuraCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AuraAbilityTypes.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Aura/Aura.h"
#include "Components/CapsuleComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "AbilitySystem/Debuff/DebuffNiagaraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/Passive/PassiveNiagaraComponent.h"
#include "Game/AuraGameInstance.h"
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"

AAuraCharacterBase::AAuraCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

	BurnDebuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>("BurnDebuffComponent");
	BurnDebuffComponent->SetupAttachment(GetRootComponent());
	BurnDebuffComponent->DebuffTag = FAuraGameplayTags::Get().Debuff_Burn;

	StunDebuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>("StunDebuffComponent");
	StunDebuffComponent->SetupAttachment(GetRootComponent());
	StunDebuffComponent->DebuffTag = FAuraGameplayTags::Get().Debuff_Stun;
	
	InvincibleBuffComponent = CreateDefaultSubobject<UDebuffNiagaraComponent>("InvincibleBuffComponent");
	InvincibleBuffComponent->SetupAttachment(GetRootComponent());
	InvincibleBuffComponent->DebuffTag = FAuraGameplayTags::Get().State_DebugInvincible;

	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Projectile, ECR_Overlap);
	GetMesh()->SetGenerateOverlapEvents(true);

	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), FName("WeaponHandSocket"));
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	EffectAttachComponent = CreateDefaultSubobject<USceneComponent>("EffectAttachPoint");
	EffectAttachComponent->SetupAttachment(GetRootComponent());

	HaloOfProtectionNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>("HaloOfProtectionComp");
	HaloOfProtectionNiagaraComponent->SetupAttachment(EffectAttachComponent);

	LifeSiphonNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>("LifeSiphonComp");
	LifeSiphonNiagaraComponent->SetupAttachment(EffectAttachComponent);

	ManaSiphonNiagaraComponent = CreateDefaultSubobject<UPassiveNiagaraComponent>("ManaSiphonComp");
	ManaSiphonNiagaraComponent->SetupAttachment(EffectAttachComponent);
}

void AAuraCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	EffectAttachComponent->SetWorldRotation(FRotator::ZeroRotator);
}

void AAuraCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	GetMesh()->bOnlyAllowAutonomousTickPose = false;
	
	OnASCRegistered.AddUObject(this, &AAuraCharacterBase::ASCRegistered);
	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(GetPlayerState()))
	{
		AuraPS->OnPlayerStateInitialized.AddDynamic(this, &AAuraCharacterBase::AuraPlayerStateInitialized);
	}
}

void AAuraCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AAuraCharacterBase, bIsStunned);
	DOREPLIFETIME(AAuraCharacterBase, bIsBurned);
	DOREPLIFETIME(AAuraCharacterBase, bInvincible);
	DOREPLIFETIME(AAuraCharacterBase, bIsBeingShock);
	DOREPLIFETIME(AAuraCharacterBase, bDead);
}

float AAuraCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float DamageTaken = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	GetOnDamageDelegate().Broadcast(DamageTaken);
	return DamageTaken;
}

UAnimMontage* AAuraCharacterBase::GetHitReactMontage_Implementation()
{
	return HitReactMontage;
}

void AAuraCharacterBase::Die(const FVector& DeathImpulse, AAuraCharacter* KilledBy)
{
	Weapon->DetachFromComponent(FDetachmentTransformRules(EDetachmentRule::KeepWorld, true));

	// 랙돌 효과
	MulticastHandleDeath(DeathImpulse);

	// 소환수 모두 제거
	RemoveAllMinions();
                
	bDead = true;
}

void AAuraCharacterBase::RemoveAllMinions()
{
	for (AActor* Minion : Minions)
	{
		if (AAuraCharacterBase* CharacterBase = Cast<AAuraCharacterBase>(Minion))
		{
			if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(CharacterBase->GetAbilitySystemComponent()))
			{
				// Gameplay Effect Context 생성
				FGameplayEffectContextHandle EffectContextHandle = AuraASC->MakeEffectContext();
				EffectContextHandle.AddSourceObject(this);
	
				// Gameplay Effect Spec 생성
				const FGameplayEffectSpecHandle EffectSpecHandle = AuraASC->MakeOutgoingSpec(DamageGameplayEffectClass, 1, EffectContextHandle);
	
				// Gameplay Effect Spec을 본인에게 적용
				AuraASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
			}
		}
	}
}

void AAuraCharacterBase::MulticastHandleDeath_Implementation(const FVector& DeathImpulse)
{
	// ��� ���� ���
	UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation(), GetActorRotation());

	// ���� ����
	Weapon->SetSimulatePhysics(true);
	Weapon->SetEnableGravity(true);
	Weapon->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	Weapon->AddImpulse(DeathImpulse * 0.1f, NAME_None, true);

	// ĳ���� ����
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetEnableGravity(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetMesh()->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	GetMesh()->AddImpulse(DeathImpulse, NAME_None, true);

	// ĳ���� �浹 ��Ȱ��ȭ
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Dissolve();
	BurnDebuffComponent->Deactivate();
	StunDebuffComponent->Deactivate();
}

void AAuraCharacterBase::StunTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bIsStunned = NewCount > 0;

	if (const UAuraAttributeSet* AuraAS = CastChecked<UAuraAttributeSet>(AttributeSet))
	{
		GetCharacterMovement()->MaxWalkSpeed = bIsStunned ? 0.f : AuraAS->GetMovementSpeed();
	}
}

void AAuraCharacterBase::StackStunTagChanged(const FGameplayEffectSpecHandle SpecHandle, int32 NewStack, int32 OldStack)
{
	
}

void AAuraCharacterBase::BeingShockedTagChanged()
{
	if (const UAuraAttributeSet* AuraAS = CastChecked<UAuraAttributeSet>(AttributeSet))
	{
		GetCharacterMovement()->MaxWalkSpeed = bIsStunned ? 0.f : AuraAS->GetMovementSpeed();
	}
}

void AAuraCharacterBase::OnRep_Stunned()
{
}

void AAuraCharacterBase::OnRep_Burned()
{
}

void AAuraCharacterBase::OnRep_Invincible()
{
}

void AAuraCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AAuraCharacterBase::InitAbilityActorInfo()
{
	
}

FVector AAuraCharacterBase::GetCombatSocketLocation_Implementation(const FGameplayTag& MontageTag)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_Weapon) && IsValid(Weapon))
	{
		return Weapon->GetSocketLocation(WeaponTipSocketName);
	}
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_LeftHand))
	{
		return GetMesh()->GetSocketLocation(LeftHandSocketName);
	}
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_RightHand))
	{
		return GetMesh()->GetSocketLocation(RightHandSocketName);
	}
	if (MontageTag.MatchesTagExact(GameplayTags.CombatSocket_Tail))
	{
		return GetMesh()->GetSocketLocation(TailSocketName);
	}
	return FVector();
}

bool AAuraCharacterBase::IsDead_Implementation() const
{
	return bDead;
}

FOnDeath* AAuraCharacterBase::GetOnDeathDelegate()
{
	return nullptr;
}

AActor* AAuraCharacterBase::GetAvatar_Implementation()
{
	return this;
}

TArray<FTaggedMontage> AAuraCharacterBase::GetAttackMontages_Implementation()
{
	return AttackMontages;
}

UNiagaraSystem* AAuraCharacterBase::GetBloodEffect_Implementation()
{
	return BloodEffect;
}

FTaggedMontage AAuraCharacterBase::GetTaggedMontageByTag_Implementation(const FGameplayTag& MontageTag)
{
	for (FTaggedMontage TaggedMontage : AttackMontages)
	{
		if (TaggedMontage.MontageTag == MontageTag)
			return TaggedMontage;
	}
	return FTaggedMontage();
}

int32 AAuraCharacterBase::GetMinionCount_Implementation()
{
	return MinionCount;
}

void AAuraCharacterBase::IncrementMinionCount_Implementation(int32 Amount)
{
	MinionCount += Amount;
}

ECharacterClass AAuraCharacterBase::GetCharacterClass_Implementation()
{
	return CharacterClass;
}

FOnASCRegistered& AAuraCharacterBase::GetOnASCRegisteredDelegate()
{
	return OnASCRegistered;
}

USkeletalMeshComponent* AAuraCharacterBase::GetWeapon_Implementation()
{
	return Weapon;
}

bool AAuraCharacterBase::IsBeingShocked_Implementation()
{
	return bIsBeingShock;
}

void AAuraCharacterBase::SetIsBeingShocked_Implementation(bool bInShock)
{
	bIsBeingShock = bInShock;
}

UAttributeSet* AAuraCharacterBase::GetAttributeSet_Implementation()
{
	return AttributeSet;
}

FOnDamageSignature& AAuraCharacterBase::GetOnDamageDelegate()
{
	return Cast<AAuraPlayerState>(GetPlayerState())->GetOnDamageDelegate();
}

void AAuraCharacterBase::ShowRangeIndicator_Implementation(bool bAttachToActor, ERangeShape RangeShape, const FVector& Location, float Radius, float Width, float Height, const FVector& RGB)
{
	if (IsLocallyControlled() == false)
		return;

	AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetController());
	if (AuraPC)
	{
		AuraPC->ShowRangeIndicator(RangeShape, Location, Radius, Width, Height, RGB);
	}
}

void AAuraCharacterBase::HideRangeIndicator_Implementation() const
{
	if (IsLocallyControlled() == false)
		return;

	AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetController());
	if (AuraPC)
	{
		AuraPC->HideRangeIndicator();
	}
}

void AAuraCharacterBase::SetCharacterInvincible_Implementation(bool InbInvincible)
{
	if (InbInvincible)
	{
		// 무적 GE 적용
		ApplyEffectToSelf(InvincibleGameplayEffectClass, 1.0f);
		
		bInvincible = InbInvincible;
	}
	else
	{
		// 캐릭터에게서 태그 제거
		if (Implements<UPlayerInterface>())
		{
			// Aura Character
			if (AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>())
			{
				if (UAuraAbilitySystemComponent* AuraASC= Cast<UAuraAbilitySystemComponent>(AuraPS->GetAbilitySystemComponent()))
				{
					const FGameplayTag& InvincibleTag = FAuraGameplayTags::Get().State_Invincible;
					FGameplayTagContainer InvincibleTagContainer;
					InvincibleTagContainer.AddTag(InvincibleTag);
					
					AuraASC->RemoveActiveEffectsWithAppliedTags(InvincibleTagContainer);
					bInvincible = InbInvincible;
				}
			}
		}
		else
		{
			// Enemy Character
			if (UAuraAbilitySystemComponent* AuraASC= Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
			{
				const FGameplayTag& InvincibleTag = FAuraGameplayTags::Get().State_Invincible;
				FGameplayTagContainer InvincibleTagContainer;
				InvincibleTagContainer.AddTag(InvincibleTag);
					
				AuraASC->RemoveActiveEffectsWithAppliedTags(InvincibleTagContainer);
	
				bInvincible = InbInvincible;
			}
		}
	}

	if (UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>())
	{
		if (UAuraAbilitySystemComponent* AuraASC= Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
		{
			if (AuraASC->HasMatchingGameplayTag(FAuraGameplayTags::Get().State_DebugInvincible))
				AuraGI->SetAuraInvincible(true);

			return;
		}
		AuraGI->SetAuraInvincible(bInvincible);
	}
}

bool AAuraCharacterBase::IsCharacterInvincible_Implementation() const
{
	return bInvincible;
}

void AAuraCharacterBase::SetCharacterInfiniteMana_Implementation(bool InbInfinite)
{
	if (InbInfinite)
	{
		// 무적 GE 적용
		ApplyEffectToSelf(InfiniteManaGameplayEffectClass, 1.0f);
	}
	else
	{
		if (UAuraAbilitySystemComponent* AuraASC= Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
		{
			const FGameplayTag& InfiniteManaTag = FAuraGameplayTags::Get().State_InfiniteMana;
			FGameplayTagContainer InfiniteManaTagContainer;
			InfiniteManaTagContainer.AddTag(InfiniteManaTag);
					
			AuraASC->RemoveActiveEffectsWithAppliedTags(InfiniteManaTagContainer);
		}
	}
}

bool AAuraCharacterBase::IsCharacterInfiniteMana_Implementation() const
{
	return GetAbilitySystemComponent()->HasMatchingGameplayTag(FAuraGameplayTags::Get().State_InfiniteMana);
}

void AAuraCharacterBase::SetCharacterDebugInvincible_Implementation(bool InbInvincible)
{
	if (InbInvincible)
	{
		// 무적 GE 적용
		ApplyEffectToSelf(DebugInvincibleGameplayEffectClass, 1.0f);
		
		bInvincible = InbInvincible;
	}
	else
	{
		// 캐릭터에게서 태그 제거
		if (Implements<UPlayerInterface>())
		{
			// Aura Character
			if (AAuraPlayerState* AuraPS = GetPlayerState<AAuraPlayerState>())
			{
				if (UAuraAbilitySystemComponent* AuraASC= Cast<UAuraAbilitySystemComponent>(AuraPS->GetAbilitySystemComponent()))
				{
					const FGameplayTag& DebugInvincibleTag = FAuraGameplayTags::Get().State_DebugInvincible;
					FGameplayTagContainer DebugInvincibleTagContainer;
					DebugInvincibleTagContainer.AddTag(DebugInvincibleTag);
					
					AuraASC->RemoveActiveEffectsWithAppliedTags(DebugInvincibleTagContainer);
					bInvincible = InbInvincible;
				}
			}
		}
		else
		{
			// Enemy Character
			if (UAuraAbilitySystemComponent* AuraASC= Cast<UAuraAbilitySystemComponent>(GetAbilitySystemComponent()))
			{
				const FGameplayTag& DebugInvincibleTag = FAuraGameplayTags::Get().State_DebugInvincible;
				FGameplayTagContainer DebugInvincibleTagContainer;
				DebugInvincibleTagContainer.AddTag(DebugInvincibleTag);
					
				AuraASC->RemoveActiveEffectsWithAppliedTags(DebugInvincibleTagContainer);
	
				bInvincible = InbInvincible;
			}
		}
	}

	if (UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>())
	{
		AuraGI->SetAuraInvincible(bInvincible);
	}
}

void AAuraCharacterBase::ASCRegistered(UAbilitySystemComponent* ASC)
{
	// UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AttributeSet);
	// if (ASC && AuraAS)
	// {
	// 	ASC->GetGameplayAttributeValueChangeDelegate(AuraAS->GetMovementSpeedAttribute()).AddUObject(
	// 		this, &AAuraCharacterBase::OnMovementSpeedChanged);
 //        
	// 	// 초기화 시점에 한 번 속도를 설정
	// 	OnMovementSpeedChanged(FOnAttributeChangeData());
	// }
}

void AAuraCharacterBase::AuraPlayerStateInitialized(AAuraPlayerState* AuraPS)
{
	// if (UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AttributeSet))
	// {
	// 	if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AuraPS->GetAbilitySystemComponent()))
	// 	{
	// 		AuraASC->GetGameplayAttributeValueChangeDelegate(AuraAS->GetMovementSpeedAttribute()).AddUObject(
	// 			this, &AAuraCharacterBase::OnMovementSpeedChanged);
 //        
	// 		// 초기화 시점에 한 번 속도를 설정
	// 		OnMovementSpeedChanged(FOnAttributeChangeData());
	// 	}
	// }
}

void AAuraCharacterBase::OnMovementSpeedChanged(const FOnAttributeChangeData& Data)
{
	// GetCharacterMovement()->MaxWalkSpeed = Data.NewValue;
}

void AAuraCharacterBase::StopMovementInput()
{
	GetMovementComponent()->StopMovementImmediately();
	GetMovementComponent()->StopActiveMovement();
}

void AAuraCharacterBase::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level) const
{
	if (!GetAbilitySystemComponent())
		return;
	
	if (!GameplayEffectClass)
		return;

	// ���ؽ�Ʈ �ڵ�
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	
	// ���ؽ�Ʈ �ڵ鿡 ������ �ҽ��� ���� <- ����Ʈ�� ���ο��� �����ϱ� ����
	ContextHandle.AddSourceObject(this);

	// ���� �ڵ�
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	
	// �����÷��� ����Ʈ�� Ÿ�ٿ��� ������
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), GetAbilitySystemComponent());
}

void AAuraCharacterBase::InitializeDefaultAttributes() const
{
	ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
	ApplyEffectToSelf(DefaultSecondaryAttributes, 1.f);
	ApplyEffectToSelf(InitializeVitalAttributes, 1.f);
	ApplyEffectToSelf(InitializeRegenAttributes, 1.f);
}

void AAuraCharacterBase::Dissolve()
{
	// �޽� ������
	if (IsValid(DissolveMaterialInstance))
	{
		// ���� ��Ƽ���� �ν��Ͻ� ����
		UMaterialInstanceDynamic* DynamicMI = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		
		// �޽ÿ� ��Ƽ���� ����
		GetMesh()->SetMaterial(0, DynamicMI);

		// Ÿ�Ӷ��� ����
		StartDissolveTimeline(DynamicMI);
	}

	// ���� ������
	if (IsValid(WeaponDissolveMaterialInstance))
	{
		// ���� ��Ƽ���� �ν��Ͻ� ����
		UMaterialInstanceDynamic* DynamicMI = UMaterialInstanceDynamic::Create(WeaponDissolveMaterialInstance, this);

		// �޽ÿ� ��Ƽ���� ����
		Weapon->SetMaterial(0, DynamicMI);

		// Ÿ�Ӷ��� ����
		StartWeaponDissolveTimeline(DynamicMI);
	}
}

void AAuraCharacterBase::AddCharacterAbilites()
{
	UAuraAbilitySystemComponent* AuraASC = CastChecked<UAuraAbilitySystemComponent>(AbilitySystemComponent);

	// 서버에서만 실행
	if (HasAuthority() == false)
		return;

	AuraASC->AddCharacterAbilities(StartupAbilities);
	AuraASC->AddCharacterPassiveAbilities(StartupPassiveAbilities);
}
