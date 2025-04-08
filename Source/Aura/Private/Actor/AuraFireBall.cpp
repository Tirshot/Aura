// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraFireBall.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayCueManager.h"
#include "AuraGameplayTags.h"
#include "Components/AudioComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

void AAuraFireBall::BeginPlay()
{
	Super::BeginPlay();

	StartOutgoingTimeline();
}

void AAuraFireBall::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 겹침 검증
	if (IsValidOverlap(OtherActor) == false)
		return;

	if (HasAuthority())
	{
		// 데미지 판정 - Gameplay Effect Handle
		if (auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
		{
			// 사망 충격파 벡터 계산
			const FVector DeathImpulse = GetActorForwardVector() * DamageEffectParams.DeathImpulseMagnitude;
			DamageEffectParams.DeathImpulse = DeathImpulse;

			// 투사체의 데미지 이펙트 파라미터에 타겟 ASC를 설정
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
	}

	if (LoopingSoundComponent)
	{
		LoopingSoundComponent->Stop();
		LoopingSoundComponent->DestroyComponent();
	}
	bHit = true;
}
