// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraSpinningBeam.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraSpinningBeamActor.h"

void UAuraSpinningBeam::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                        const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                        const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAuraSpinningBeam::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 아바타 액터의 이동속도 복구
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraSpinningBeam::SpawnBeam()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
		return;
	
	// 빔 갯수에 맞춰 일정한 각도로 분포한 씬 컴포넌트를 생성하고 배열에 보관
	TArray<FRotator> Rotators = UAuraAbilitySystemLibrary::EvenlySpacedRotators(
		AvatarActor->GetActorForwardVector(), FVector::UpVector, 360.f, BeamCount);
	
	for (int i = 0; i < BeamCount; i++)
	{
		FVector Direction = Rotators[i].Vector();
		float Angle = FMath::DegreesToRadians((360.f / BeamCount) * i);
		
		FTransform SpawnTransform;

		SpawnTransform.SetLocation(AvatarActor->GetActorLocation() + Direction * OrbitRadius);
		SpawnTransform.SetRotation(Rotators[i].Quaternion());

		// 월드에 미사일 생성
		AAuraSpinningBeamActor* Beam = GetWorld()->SpawnActorDeferred<AAuraSpinningBeamActor>(
			BeamClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			GetAvatarActorFromActorInfo()->GetInstigatorController()->GetPawn(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		// 파라미터 전달
		Beam->InitialAngle = Angle;
		Beam->OrbitRadius = OrbitRadius;
		Beam->AngleSpeed = AngleSpeed;
		Beam->DamageDeltaTime = DamageDeltaTime;

		// 소유 어빌리티 지정
		Beam->SetOwnedAbility(this);
		
		// 클래스 디폴트의 데미지 값 적용
		Beam->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
		Beam->SetOwner(GetAvatarActorFromActorInfo());

		LightningBeams.Add(Beam);

		Beam->FinishSpawning(SpawnTransform);
		Beam->SetLifeSpan(BeamLifeSpan);
		
		FGameplayCueParameters CueParams;
		CueParams.SourceObject = Beam;
		CueParams.TargetAttachComponent = AvatarActor->GetRootComponent();
	}
}
