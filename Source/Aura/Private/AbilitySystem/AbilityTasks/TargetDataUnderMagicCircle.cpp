// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/TargetDataUnderMagicCircle.h"

#include "AbilitySystemComponent.h"
#include "Aura/Aura.h"
#include "Player/AuraPlayerController.h"

UTargetDataUnderMagicCircle* UTargetDataUnderMagicCircle::CreateTargetDataUnderMagicCircle(
	UGameplayAbility* OwningAbility)
{
	if (!OwningAbility)
		return nullptr;
	
	UTargetDataUnderMagicCircle* MyObj = NewAbilityTask<UTargetDataUnderMagicCircle>(OwningAbility);
	if (MyObj)
		return MyObj;
	
	return nullptr;
}

void UTargetDataUnderMagicCircle::Activate()
{
	const bool bIsLocallyControlled = Ability->GetCurrentActorInfo()->IsLocallyControlled();
	if (bIsLocallyControlled)
	{
		// 클라에서 바로 데이터 전송
		SendMagicCircleData();
	}
	else
	{
		// 서버에서 데이터 대기
		if (!AbilitySystemComponent.IsValid())
			return;

		FGameplayAbilitySpecHandle SpecHandle = GetAbilitySpecHandle();
		FPredictionKey ActivationPredictionKey = GetActivationPredictionKey();
		
		AbilitySystemComponent.Get()->AbilityTargetDataSetDelegate(SpecHandle, ActivationPredictionKey).AddUObject(this, &UTargetDataUnderMagicCircle::OnTargetDataReplicatedCallback);
		
		const bool bCalledDelegate = AbilitySystemComponent->CallReplicatedTargetDataDelegatesIfSet(SpecHandle, ActivationPredictionKey);
		if (bCalledDelegate == false)
		{
			SetWaitingOnRemotePlayerData();
		}
	}
}

void UTargetDataUnderMagicCircle::SendMagicCircleData()
{
	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent.Get());

	APlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.Get();
	if (!PC)
		return;
	
	AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(PC);
	if (!AuraPC)
		return;
	
	FHitResult HitResult;
	AuraPC->GetHitResultUnderMagicCircle(ECC_GroundCheck, false, HitResult);

	FGameplayAbilityTargetDataHandle DataHandle;
	FGameplayAbilityTargetData_SingleTargetHit* Data = new FGameplayAbilityTargetData_SingleTargetHit();
	Data->HitResult = HitResult;
	DataHandle.Add(Data);

	AbilitySystemComponent->ServerSetReplicatedTargetData(
		GetAbilitySpecHandle(),
		GetActivationPredictionKey(),
		DataHandle,
		FGameplayTag(),
		AbilitySystemComponent->ScopedPredictionKey);

	if (ShouldBroadcastAbilityTaskDelegates())
	{
		ValidData.Broadcast(DataHandle);
	}
}

void UTargetDataUnderMagicCircle::OnTargetDataReplicatedCallback(const FGameplayAbilityTargetDataHandle& DataHandle,
	FGameplayTag ActivationTag)
{
	AbilitySystemComponent->ConsumeClientReplicatedTargetData(GetAbilitySpecHandle(), GetActivationPredictionKey());
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		ValidData.Broadcast(DataHandle);
	}
}
