// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraBeamSpell.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Interaction/CombatInterface.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AbilityTasks/TargetDataUnderMouse.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "GameFramework/CharacterMovementComponent.h"

UAuraBeamSpell::UAuraBeamSpell()
{
	SpellType = ESpellType::Targeting;
}

void UAuraBeamSpell::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (!AvatarActor || !AvatarActor->Implements<UCombatInterface>())
	{
		K2_CancelAbility();
		return;
	}
	
	UTargetDataUnderMouse* TargetDataUnderMouse = UTargetDataUnderMouse::CreateTargetDataUnderMouse(this);
	TargetDataUnderMouse->ValidData.AddDynamic(this, &UAuraBeamSpell::OnTargetDataReceived);
	TargetDataUnderMouse->ReadyForActivation();
}

void UAuraBeamSpell::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageCostTimerHandle);
	}
	
	DamageCostTimerHandle.Invalidate();
	
	if (OwnerPlayerController)
	{
		OwnerPlayerController->bShowMouseCursor = true;
	}
	
	if (OwnerCharacter)
	{
		ICombatInterface::Execute_SetInShockLoop(OwnerCharacter, false);
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	
	if (bFirstTargetHasCombatInterface)
	{
		if (MouseHitActor)
		{
			ICombatInterface::Execute_SetIsBeingShocked(MouseHitActor, false);
			if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MouseHitActor))
			{
				ASC->RemoveGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShockLoop"));
			
				// 마지막 한 틱 데미지 적용
				FDamageEffectParams Params = MakeDamageEffectParamsFromClassDefaults(MouseHitActor);
				Params.TargetAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MouseHitActor);
				UAuraAbilitySystemLibrary::ApplyDamageEffect(Params, HitResult);
			}
		}
	}
	else
	{
		if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerCharacter))
		{
			ASC->RemoveGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShockLoop"));
		}
	}
	
	for (const auto& Target : AdditionalTargetActors)
	{
		ICombatInterface::Execute_SetIsBeingShocked(Target, false);
		
		if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
		{
			ASC->RemoveGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShockLoop"));
		}
		
		// 마지막 한 틱 데미지 적용
		FDamageEffectParams Params = MakeDamageEffectParamsFromClassDefaults(Target);
		Params.TargetAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
		
		FHitResult AdditionalHitResult;
		AdditionalHitResult.ImpactPoint = Target->GetActorLocation();
		UAuraAbilitySystemLibrary::ApplyDamageEffect(Params, AdditionalHitResult);
	}
	
	// 쿨다운 적용
	K2_CommitAbilityCooldown();
	
	AdditionalTargetActors.Reset();
	MouseHitActor = nullptr;
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraBeamSpell::OnTargetDataReceived(const FGameplayAbilityTargetDataHandle& DataHandle)
{
	HitResult = *DataHandle.Data[0]->GetHitResult();
	StoreMouseDataInfo(HitResult);
	
	if (!HitResult.bBlockingHit)
	{
		K2_CancelAbility();
		return;
	}
	
	StoreOwnerVariables();
	
	if (!OwnerPlayerController || !OwnerCharacter)
	{
		K2_CancelAbility();
		return;
	}
	
	// 마우스 커서 숨김
	OwnerPlayerController->bShowMouseCursor = false;
	
	FGameplayEffectContextHandle ContextHandle;
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(OwnerCharacter, OwnerCharacter);
	
	K2_ExecuteGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShockBurst"), ContextHandle);
	
	ICombatInterface::Execute_UpdateFacingTarget(OwnerCharacter, MouseHitLocation);
	
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, 
		TEXT("PlayAttackMontage"), 
		AttackMontage, 
		1.0f, 
		NAME_None, 
		true
	);

	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UAuraBeamSpell::OnMontageCompleted);
		MontageTask->OnBlendOut.AddDynamic(this, &UAuraBeamSpell::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UAuraBeamSpell::OnMontageInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &UAuraBeamSpell::OnMontageInterrupted);

		MontageTask->ReadyForActivation();
	}
	else
	{
		K2_EndAbility();
	}
	
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		FGameplayTag::RequestGameplayTag("Event.Montage.Electrocute"),
		nullptr,         
		true,           // Only Trigger Once
		true            // Matches Exact
	);
	
	WaitEventTask->EventReceived.AddDynamic(this, &UAuraBeamSpell::OnMontageTagReceived);
	WaitEventTask->ReadyForActivation();
	
	UAbilityTask_WaitInputRelease* InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	
	InputReleaseTask->OnRelease.AddDynamic(this, &UAuraBeamSpell::OnInputReleased);
	InputReleaseTask->ReadyForActivation();
}

void UAuraBeamSpell::OnMontageCompleted()
{
}

void UAuraBeamSpell::OnMontageInterrupted()
{
	K2_EndAbility();
}

void UAuraBeamSpell::OnMontageTagReceived(FGameplayEventData Payload)
{
	CheckAbilityUpgrades();
	
	// 사용자에게 Shock Loop 상태를 설정, 이동 금지
	ICombatInterface::Execute_SetInShockLoop(OwnerCharacter, true);
	OwnerCharacter->GetCharacterMovement()->DisableMovement();
	
	// 빔 소환
	TraceFirstTarget(MouseHitLocation);
	
	// 빔의 시작 부분 큐 지정
	USkeletalMeshComponent* WeaponComp = ICombatInterface::Execute_GetWeapon(OwnerCharacter);
	FirstTargetCueParams.Location = MouseHitLocation;
	FirstTargetCueParams.SourceObject = MouseHitActor;
	FirstTargetCueParams.TargetAttachComponent = WeaponComp;
	
	// 빔의 끝 부분이 몬스터 or 스태틱 메시를 공격
	AActor* CueTarget = nullptr;
	bFirstTargetHasCombatInterface = MouseHitActor->Implements<UCombatInterface>();
	
	if (bFirstTargetHasCombatInterface)
	{
		// 몬스터, 보스 등
		CueTarget = MouseHitActor;
	}
	else
	{
		// 일반 스태틱 메시(구조물, 바닥, 벽 등)
		CueTarget = GetAvatarActorFromActorInfo();
	}

	if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CueTarget))
	{
		ASC->AddGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShockLoop"), FirstTargetCueParams);
	}
	
	if (bFirstTargetHasCombatInterface)
	{
		ICombatInterface::Execute_SetIsBeingShocked(CueTarget, true);
		
		// 추가 타겟 추적
		StoreAdditionalTarget(AdditionalTargetActors);
		
		for (const auto& Target : AdditionalTargetActors)
		{
			ICombatInterface::Execute_SetIsBeingShocked(Target, true);
			
			FGameplayCueParameters AdditionalCueParams;
			AdditionalCueParams.Location = Target->GetActorLocation();
			AdditionalCueParams.SourceObject = Target;
			AdditionalCueParams.TargetAttachComponent = CueTarget->GetRootComponent();
			
			if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target))
			{
				ASC->AddGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShockLoop"), AdditionalCueParams);
			}
		}
	}
	
	// 델타 타임마다 데미지 입힘
	GetWorld()->GetTimerManager().SetTimer(
		DamageCostTimerHandle, 
		[this]()
		{
			if (!IsActive()) 
			{
				// 이미 종료된 상태라면 타이머 클리어
				GetWorld()->GetTimerManager().ClearTimer(DamageCostTimerHandle);
				return;
			}
        
			this->ApplyDamage(); 
		}, 
		DamageDeltaTime, 
		true
	);
}

void UAuraBeamSpell::OnInputReleased(float TimeHeld)
{
	if (TimeHeld < MinSpellTime)
	{
		UKismetSystemLibrary::Delay(OwnerPlayerController, MinSpellTime-TimeHeld, FLatentActionInfo());
	}
	
	K2_EndAbility();
}

void UAuraBeamSpell::ApplyDamage()
{
	if (!IsActive())
		return;
	
	if (K2_CommitAbilityCost())
	{
		if (MouseHitActor && MouseHitActor->Implements<UCombatInterface>() && UAuraAbilitySystemLibrary::IsNotFriend(MouseHitActor, OwnerCharacter))
		{
			FDamageEffectParams Params = MakeDamageEffectParamsFromClassDefaults();
			Params.TargetAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MouseHitActor);
			UAuraAbilitySystemLibrary::ApplyDamageEffect(Params, HitResult);
		}
	
		for (const auto& Target : AdditionalTargetActors)
		{
			if (Target->Implements<UCombatInterface>() && UAuraAbilitySystemLibrary::IsNotFriend(Target, OwnerCharacter))
			{
				FDamageEffectParams Params = MakeDamageEffectParamsFromClassDefaults();
				Params.TargetAbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
				FHitResult AdditionalHitResult;
				AdditionalHitResult.ImpactPoint = Target->GetActorLocation();
				UAuraAbilitySystemLibrary::ApplyDamageEffect(Params, AdditionalHitResult);
			}
		}
	}
	else
	{
		K2_EndAbility();
	}
}

void UAuraBeamSpell::StoreMouseDataInfo(const FHitResult& InHitResult)
{
	HitResult = InHitResult;
	
	if (HitResult.bBlockingHit)
	{
		MouseHitLocation = HitResult.ImpactPoint;
		MouseHitActor = HitResult.GetActor();
	}
	else
	{
		MouseHitLocation = FVector::ZeroVector;
		MouseHitActor = nullptr;

		CancelAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true);
	}
}

void UAuraBeamSpell::StoreOwnerVariables()
{
	// ���� ����
	if (CurrentActorInfo)
	{
		OwnerPlayerController = CurrentActorInfo->PlayerController.Get();
		OwnerCharacter = Cast<ACharacter>(CurrentActorInfo->AvatarActor);
	}
}

void UAuraBeamSpell::TraceFirstTarget(const FVector& BeamTargetLocation)
{
	if (!OwnerCharacter)
		return;

	if (MouseHitActor == OwnerCharacter)
		return;

	// ���� ��������
	if (OwnerCharacter->Implements<UCombatInterface>())
	{
		if (auto* Weapon = ICombatInterface::Execute_GetWeapon(OwnerCharacter))
		{
			// ��ü Ʈ���̽�
			TArray<AActor*> ActorsToIgnore;
			ActorsToIgnore.Add(OwnerCharacter);

			const FVector SocketLocation = Weapon->GetSocketLocation(FName("TipSocket"));

			UKismetSystemLibrary::SphereTraceSingle(
				OwnerCharacter,
				SocketLocation,
				BeamTargetLocation,
				10.f,
				ETraceTypeQuery::TraceTypeQuery1,
				false,
				ActorsToIgnore,
				EDrawDebugTrace::None,
				HitResult,
				true
				);

			// ���� ��ġ �� Ÿ�� ������Ʈ
			if (HitResult.bBlockingHit)
			{
				MouseHitLocation = HitResult.ImpactPoint;
				MouseHitActor = HitResult.GetActor();
			}
		}
	}
	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(MouseHitActor))
	{
		if (!CombatInterface->GetOnDeathDelegate()->IsAlreadyBound(this, &UAuraBeamSpell::PrimaryTargetDied))
		{
			CombatInterface->GetOnDeathDelegate()->AddDynamic(this, &UAuraBeamSpell::PrimaryTargetDied);
		}
	}
}

void UAuraBeamSpell::StoreAdditionalTarget(TArray<AActor*>& OutAdditionalTargets)
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetAvatarActorFromActorInfo());
	ActorsToIgnore.Add(MouseHitActor);

	// ���� ������
	TArray<AActor*> OverlappingActors;
	UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
		GetAvatarActorFromActorInfo(),
		OverlappingActors,
		ActorsToIgnore,
		850.f,
		MouseHitActor->GetActorLocation());

	AdditionalTargets += GetAbilityLevel() - 1;
	int32 NumAdditionalTargets = FMath::Min(AdditionalTargets, MaxNumShockTargets);
	//int32 NumAdditionalTargets = 5;

	// ���� ������ ��� ã��
	UAuraAbilitySystemLibrary::GetClosestTargets(
		NumAdditionalTargets,
		OverlappingActors,
		OutAdditionalTargets,
		MouseHitActor->GetActorLocation());

	for (AActor* Target : OutAdditionalTargets)
	{
		if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(Target))
		{
			if (CombatInterface->GetOnDeathDelegate()->IsAlreadyBound(this, &UAuraBeamSpell::AdditionalTargetDied) == false)
			{
				CombatInterface->GetOnDeathDelegate()->AddDynamic(this, &UAuraBeamSpell::AdditionalTargetDied);
			}
		}
	}
}

void UAuraBeamSpell::PrimaryTargetDied(AActor* DeadActor)
{
	if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(DeadActor))
	{
		ASC->RemoveGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShockLoop"));
		K2_EndAbility();
	}
}

void UAuraBeamSpell::AdditionalTargetDied(AActor* DeadActor)
{
	if (auto* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(DeadActor))
	{
		ASC->RemoveGameplayCue(FGameplayTag::RequestGameplayTag("GameplayCue.ShockLoop"));
	}
	AdditionalTargetActors.Remove(DeadActor);
}
