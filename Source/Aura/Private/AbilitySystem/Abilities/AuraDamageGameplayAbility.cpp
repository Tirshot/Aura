// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraDamageGameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Character/AuraCharacter.h"
#include "Interaction/CombatInterface.h"
#include "Player/AuraPlayerController.h"
#include "UI/WidgetController/OverlayWidgetController.h"

void UAuraDamageGameplayAbility::CauseDamage(AActor* TargetActor)
{
	FGameplayEffectSpecHandle DamageSpecHandle = MakeOutgoingGameplayEffectSpec(DamageEffectClass, 1.f);

	float DamageMagnitude = Damage.GetValueAtLevel(GetAbilityLevel());
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, DamageType, DamageMagnitude);

	GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*DamageSpecHandle.Data.Get(), UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor));
}

void UAuraDamageGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAuraDamageGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                            bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraDamageGameplayAbility::StopAutoRun()
{
	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
		{
			if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(AvatarPawn->GetController()))
			{
				AuraPC->StopAutoRun();
			}
		}
	}
}

FDamageEffectParams UAuraDamageGameplayAbility::MakeDamageEffectParamsFromClassDefaults(AActor* TargetActor, FVector InRadialDamageOrigin, bool bOverrideKnockbackDirection, FVector InKnockbackDirectionOverride, bool bOverrideDeathImpulse, FVector DeathImpulseDirectionOverride, bool bOverridePitch, float PitchOverride) const
{
	FDamageEffectParams Params;

	// ASC
	Params.WorldContextObject = GetAvatarActorFromActorInfo();
	Params.DamageGameplayEffectClass = DamageEffectClass;
	Params.SourceAbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	Params.TargetAbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	
	// 데미지
	Params.BaseDamage = Damage.GetValueAtLevel(GetAbilityLevel());
	Params.AbilityLevel = GetAbilityLevel();
	Params.DamageType = DamageType;
	Params.MagicPowerCoefficient = MagicPowerCoefficient.GetValue();

	// 디버프
	Params.DebuffChance = DebuffChance;
	Params.DebuffDamage = DebuffDamage;
	Params.DebuffDuration = DebuffDuration;
	Params.DebuffFrequency = DebuffFrequency;

	// 충격파
	Params.DeathImpulseMagnitude = DeathImpulseMagnitude;

	// 넉백
	Params.KnockbackForceMagnitude = KnockbackForceMagnitude;
	Params.KnockbackChance = KnockbackChance;

	// 넉백 계산
	const bool bKnockback = FMath::RandRange(1, 100) < Params.KnockbackChance;

	// 타겟 방향으로 충격파와 넉백 실시
	if (IsValid(TargetActor))
	{
		FRotator Rotation = (TargetActor->GetActorLocation() - GetAvatarActorFromActorInfo()->GetActorLocation()).Rotation();
		if (bOverridePitch)
		{
			Rotation.Pitch = PitchOverride;
		}
		FVector ToTarget = Rotation.Vector();

		if (bOverrideKnockbackDirection == false)
		{
			Params.KnockbackForce = ToTarget * KnockbackForceMagnitude;
		}

		if (bOverrideDeathImpulse == false)
		{
			Params.DeathImpulse = ToTarget * DeathImpulseMagnitude;
		}
	}

	if (bKnockback == false)
	{
		Params.KnockbackForce = FVector::ZeroVector;
	}

	// 넉백 방향 오버라이드
	if (bOverrideKnockbackDirection && bKnockback)
	{
		// 백터 정규화 이후 크기 계산
		InKnockbackDirectionOverride.Normalize();
		Params.KnockbackForce = InKnockbackDirectionOverride * KnockbackForceMagnitude;

		// 피치 오버라이드
		if (bOverridePitch)
		{
			FRotator KnockbackRotation = InKnockbackDirectionOverride.Rotation();
			KnockbackRotation.Pitch = PitchOverride;
			Params.KnockbackForce = KnockbackRotation.Vector() * KnockbackForceMagnitude;
		}
	}

	// 충격파 방향 오버라이드
	if (bOverrideDeathImpulse)
	{
		DeathImpulseDirectionOverride.Normalize();
		Params.DeathImpulse = DeathImpulseDirectionOverride * DeathImpulseMagnitude;

		if (bOverridePitch)
		{
			FRotator DeathImpulseRotation = DeathImpulseDirectionOverride.Rotation();
			DeathImpulseRotation.Pitch = PitchOverride;
			Params.DeathImpulse = DeathImpulseRotation.Vector() * DeathImpulseMagnitude;
		}
	}

	// 범위 공격 판정
	if (bIsRadialDamage)
	{
		Params.bIsRadialDamage = bIsRadialDamage;
		Params.RadialDamageOrigin = InRadialDamageOrigin;
		Params.RadialDamageInnerRadius = RadialDamageInnerRadius;
		Params.RadialDamageOuterRadius = RadialDamageOuterRadius;
	}

	return Params;
}

float UAuraDamageGameplayAbility::GetDamageAtLevel() const
{
	return Damage.GetValueAtLevel(GetAbilityLevel());
}

void UAuraDamageGameplayAbility::RemoveRangeSpellHelpMessage(AActor* AvatarActor)
{
	if (AvatarActor == nullptr)
		return;

	AAuraCharacter* Aura = Cast<AAuraCharacter>(AvatarActor);
	if (Aura == nullptr)
		return;

	UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AvatarActor));
	if (AuraASC == nullptr)
		return;
	
	// 범위 스킬인지 체크
	if (SpellType != ESpellType::Ranged)
		return;

	AuraASC->MessageRemove(FGameplayTag::RequestGameplayTag("GameplayCue.Message.WaitForExecute"));
}

void UAuraDamageGameplayAbility::ShowMagicCircle(float MagicCircleRadius)
{
	// 매직 서클
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
		return;

	if (AvatarActor->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_ShowMagicCircle(AvatarActor, nullptr, AbilityRange, MagicCircleRadius);
	}
}

void UAuraDamageGameplayAbility::ShowRangeIndicator(ERangeShape Shape, float Width)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
		return;
	
	// 범위 표시기
	if (AvatarActor->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_ShowRangeIndicator(
			AvatarActor,
			true,
			Shape,
			AvatarActor->GetActorUpVector(),
			AbilityRange,
			Width,
			AbilityRange / 2,
			FVector(3,3,3));
	}
}

void UAuraDamageGameplayAbility::HideMagicCircleAndRangeIndicator()
{
	// 매직 서클 숨기기
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
		return;

	RemoveRangeSpellHelpMessage(AvatarActor);
	
	if (AvatarActor->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_HideMagicCircle(AvatarActor);
	}

	// 범위 표시기 숨기기
	if (AvatarActor->Implements<UCombatInterface>())
	{
		ICombatInterface::Execute_HideRangeIndicator(AvatarActor);
	}
}

AAbilityRangeIndicator* UAuraDamageGameplayAbility::SpawnRangeIndicator(const FVector& Location, bool bAttachToActor, ERangeShape RangeShape, float Radius, float Width, float Height, FVector RGB, float LifeSpan)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
		return nullptr;
        
	auto* RangeIndicator = GetWorld()->SpawnActor<AAbilityRangeIndicator>();
	RangeIndicator->SetOwner(AvatarActor);
	
	if (LifeSpan == 0.f)
		LifeSpan = 1.5f;
	RangeIndicator->SetLifeSpan(LifeSpan);
	
	if (RGB == FVector::ZeroVector)
		RGB = FVector(5.f, 0.f, 0.f);
	
	RangeIndicator->IndicatorInitialized.Broadcast(AvatarActor, bAttachToActor, RangeShape, Location, Radius, Width, Height, 0.f, RGB);

	return RangeIndicator;
}

FVector UAuraDamageGameplayAbility::ReceivedMouseHitResult(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!TargetDataHandle.IsValid(0))
		return FVector();

	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0);
	const FHitResult* HitResult = TargetData->GetHitResult();

	// 마우스 히트 정보를 멤버 변수로 만듬
	CurrentTargetLocation = HitResult->ImpactPoint;

	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
		{
			if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(AvatarPawn->GetController()))
			{
				return CurrentTargetLocation = AuraPC->GetMagicCircleLocation();
			}
		}
	}
	return FVector();
}

FTaggedMontage UAuraDamageGameplayAbility::GetRandomTaggedMontageFromArray(const TArray<FTaggedMontage>& TaggedMontages) const
{
	if (TaggedMontages.Num() > 0)
	{
		const int32 Selection = FMath::RandRange(0, TaggedMontages.Num() - 1);
		return TaggedMontages[Selection];
	}
	return FTaggedMontage();
}


