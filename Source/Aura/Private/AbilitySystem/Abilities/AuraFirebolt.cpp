
#include "AbilitySystem/Abilities/AuraFirebolt.h"

#include "Interaction/CombatInterface.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraProjectile.h"

FString UAuraFirebolt::GetDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	if (Level == 1)
	{
		return FString::Printf(TEXT(
			"<Title>파이어볼트</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			ScaledDamage
			);
	}
	else
	{
		return FString::Printf(TEXT(
			"<Title>파이어볼트</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 %d개 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			FMath::Min(NumProjectiles, Level),
			ScaledDamage
		);
	}
}

FString UAuraFirebolt::GetNextLevelDescription(int32 Level)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 %d개 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		FMath::Min(NumProjectiles, Level),
		ScaledDamage
	);
}

void UAuraFirebolt::SpawnProjectiles(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag, bool bOverridePitch, float PitchOverride, AActor* HomingTarget)
{
	// 서버 상에 있는지 확인
	const bool bIsServer = GetAvatarActorFromActorInfo()->HasAuthority();
	if (!bIsServer)
		return;

	// 무기의 소켓 위치 가져오기
	const FVector SocketLocation = ICombatInterface::Execute_GetCombatSocketLocation(
		GetAvatarActorFromActorInfo(),
		SocketTag);

	// 피치 오버라이드
	FRotator Rotation = (ProjectileTargetLocation - SocketLocation).Rotation();
	if (bOverridePitch)
		Rotation.Pitch = PitchOverride;

	/* 
	* 다수의 투사체 퍼트리기
	*/

	const FVector Forward = Rotation.Vector();

	TArray<FRotator> Rotators = UAuraAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, ProjectileSpread, NumProjectiles);

	for (const FRotator& Rot : Rotators)
	{
		FTransform SpawnTransform;
		SpawnTransform.SetLocation(SocketLocation);
		SpawnTransform.SetRotation(Rot.Quaternion());

		AAuraProjectile* Projectile = GetWorld()->SpawnActorDeferred<AAuraProjectile>(
			ProjectileClass,
			SpawnTransform,
			GetOwningActorFromActorInfo(),
			Cast<APawn>(GetOwningActorFromActorInfo()),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

		Projectile->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();

		Projectile->FinishSpawning(SpawnTransform);
	}
}
