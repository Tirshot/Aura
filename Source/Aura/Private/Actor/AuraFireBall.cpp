// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraFireBall.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueManager.h"
#include "AuraGameplayTags.h"
#include "GameplayCueNotify_Static.h"
#include "Components/AudioComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

void AAuraFireBall::BeginPlay()
{
	Super::BeginPlay();

	StartOutgoingTimeline();
}

void AAuraFireBall::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// ��ħ ����
	if (IsValidOverlap(OtherActor) == false)
		return;

	if (HasAuthority())
	{
		// ������ ���� - Gameplay Effect Handle
		if (auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			// ��� ����� ���� ���
			const FVector DeathImpulse = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;
			DamageEffectParams.DeathImpulse = DeathImpulse;

			// ����ü�� ������ ����Ʈ �Ķ���Ϳ� Ÿ�� ASC�� ����
			DamageEffectParams.TargetAbilitySystemComponent = TargetASC;
			UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
		}
	}
}

void AAuraFireBall::OnHit()
{
	if (GetOwner())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = GetActorLocation();
		UGameplayCueManager::ExecuteGameplayCue_NonReplicated(GetOwner(), FAuraGameplayTags::Get().GameplayCue_FireBlast, CueParams);
		
		// if (LoopingSoundComponent)
		// {
		// 	LoopingSoundComponent->Stop();
		// 	LoopingSoundComponent->DestroyComponent();
		// }
		bHit = true;
	}
}
