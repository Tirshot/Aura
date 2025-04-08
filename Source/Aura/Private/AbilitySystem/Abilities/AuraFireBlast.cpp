// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraFireBlast.h"
#include "Actor/AuraFireBall.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

FString UAuraFireBlast::GetDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Title>화염 폭발</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>지정한 범위 내에 %d의 화염구를 소환하여 </><Damage>%d</><Default>의 방사형 피해를 입히는 화염 폭풍을 소환합니다. 일정 확률로 적에게 화상을 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		NumFireBalls,
		ScaledDamage
	);
}

FString UAuraFireBlast::GetNextLevelDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>지정한 범위 내에 %d의 화염구를 소환하여</><Damage>%d</><Default>의 방사형 피해를 입히는 화염 폭풍을 소환합니다. 일정 확률로 적에게 화상을 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		NumFireBalls,
		ScaledDamage
	);
}

TArray<AAuraFireBall*> UAuraFireBlast::SpawnFireBalls()
{
	TArray<AAuraFireBall*> FireBalls;

	const FVector Forward = GetAvatarActorFromActorInfo()->GetActorForwardVector();
	const FVector Location = GetAvatarActorFromActorInfo()->GetActorLocation();

	// 균일하게 회전하는 로테이터 가져오기
	TArray<FRotator> Rotators = UAuraAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, 360.f, NumFireBalls);

	for (const FRotator& Rotator : Rotators)
	{
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(Location);
		SpawnTransform.SetRotation(Rotator.Quaternion());

		// 월드에 파이어볼 생성
		AAuraFireBall* FireBall = GetWorld()->SpawnActorDeferred<AAuraFireBall>(
			FireBallClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			CurrentActorInfo->PlayerController->GetPawn(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		FireBall->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
		FireBall->ReturnToActor = GetAvatarActorFromActorInfo();
		FireBall->SetOwner(GetAvatarActorFromActorInfo());
		FireBall->ExplosionDamageParams = MakeDamageEffectParamsFromClassDefaults();

		// 클래스 디폴트의 데미지 값 적용
		FireBalls.Add(FireBall);

		FireBall->FinishSpawning(SpawnTransform);
	}
	return FireBalls;
}
