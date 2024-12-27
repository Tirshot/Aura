// Fill out your copyright notice in the Description page of Project Settings.

#include "Actor/AuraEffectActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"


AAuraEffectActor::AAuraEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// 메시와 형태는 블루프린트에서 지정 - 기능은 C++에서 지정
	
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("SceneRoot"));

}

void AAuraEffectActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAuraEffectActor::ApplyEffectToTarget(AActor *TargetActor, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{
	// 몬스터인지 확인
	if (TargetActor->ActorHasTag(FName("Enemy")) && !bApplyEffectsToEnemy)
		return;

	// 타겟의 어빌리티 시스템 인터페이스 가져옴
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (TargetASC == nullptr)
		return;
	
	check(GameplayEffectClass);
	// Gameplay Effect Context 생성
	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();

	// 이펙트 발생자에 대한 정보 추가
	EffectContextHandle.AddSourceObject(this);

	// Gameplay Effect Spec 생성
	const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass, ActorLevel, EffectContextHandle);
	
	// Gameplay Effect Spec을 본인에게 적용
	const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());

	// 이펙트 스펙 핸들을 참조하여 Infinite Effect 확인
	const bool bIsInfinite = EffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;
	if (bIsInfinite && InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC);
	}

	if (bDestroyOnEffectApplication && EffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy != EGameplayEffectDurationType::Infinite)
		Destroy();
}

void AAuraEffectActor::OnOverlap(AActor *TargetActor)
{
	// 몬스터인지 확인
	if (TargetActor->ActorHasTag(FName("Enemy")) && !bApplyEffectsToEnemy)
		return;

	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
}

void AAuraEffectActor::OnEndOverlap(AActor *TargetActor)
{
	// 몬스터인지 확인
	if (TargetActor->ActorHasTag(FName("Enemy")) && !bApplyEffectsToEnemy)
		return;

	if (InstantEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InstantGameplayEffectClass);
	}
	if (DurationEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplicationPolicy == EEffectApplicationPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(TargetActor, InfiniteGameplayEffectClass);
	}
	if (InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::RemoveOnEndOverlap)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (!TargetASC)
			return;
		
		// Map 컨테이너에서 제거할 핸들
		TArray<FActiveGameplayEffectHandle> HandlesToRemove;

		// 활성화 된 Infinite 이펙트 Map 컨테이너 순회
		for (TPair<FActiveGameplayEffectHandle, UAbilitySystemComponent *> HandlePair : ActiveEffectHandles)
		{
			// 동일한 ASC이면
			if (TargetASC == HandlePair.Value)
			{
				// 게임플레이 이펙트 제거
				TargetASC->RemoveActiveGameplayEffect(HandlePair.Key, 1);
				HandlesToRemove.Add(HandlePair.Key);
			}
		}

		// Map 컨테이너에서 이펙트 제거
		for (auto& Handle : HandlesToRemove)
		{
			ActiveEffectHandles.FindAndRemoveChecked(Handle);
		}
	}
}
