// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraArcaneAreaAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraArcaneArea.h"
#include "Character/AuraCharacterBase.h"

void UAuraArcaneAreaAbility::SpawnArcaneArea(const FVector& Location)
{
	const FRotator& Rotation = GetAvatarActorFromActorInfo()->GetActorRotation();
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Instigator = GetAvatarActorFromActorInfo()->GetInstigator();
	SpawnParams.Owner = GetAvatarActorFromActorInfo();
	
	ArcaneArea = Cast<AAuraArcaneArea>(GetWorld()->SpawnActor(ArcaneAreaClass, &Location, &Rotation, SpawnParams));

	ArcaneArea->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	ArcaneArea->OnDestroyed.AddDynamic(this, &UAuraArcaneAreaAbility::OnArcaneAreaDestroyed);
}

void UAuraArcaneAreaAbility::OnArcaneAreaDestroyed(AActor* DestroyedActor)
{
	K2_EndAbility();
}

void UAuraArcaneAreaAbility::ApplySlowEffect(AActor* TargetActor)
{
	float MovementSpeed = UAuraAbilitySystemLibrary::GetAttributeValue(TargetActor, FAuraGameplayTags::Get().Attributes_Secondary_MovementSpeed);
	float Magnitude = MovementSpeed * SlowDownRatio;
	
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponentFromActorInfo()->MakeOutgoingSpec(SlowDownEffectClass, GetAbilityLevel(), FGameplayEffectContextHandle());
	SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.Movement"), Magnitude);

	if (auto* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
	{
		auto ActiveGEHandle = GetAbilitySystemComponentFromActorInfo()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

		// // 스택 델리게이트 생성 및 바인딩
		// FOnActiveGameplayEffectStackChange OnSlowStackChangedDelegate = FOnActiveGameplayEffectStackChange::AddUFunction(this, &UAuraArcaneAreaAbility::OnSlowStackChanged);
		// GetAbilitySystemComponentFromActorInfo()->OnGameplayEffectStackChangeDelegate(ActiveGEHandle, OnSlowStackChangedDelegate);
	}
	
	
}

void UAuraArcaneAreaAbility::OnSlowStackChanged(FGameplayEffectSpecHandle SpecHandle, int32 NewStack, int32 OldStack)
{
}
