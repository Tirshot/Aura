
#include "AbilitySystem/Abilities/AuraFirebolt.h"

#include "AuraGameplayTags.h"
#include "Interaction/CombatInterface.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"

UAuraFirebolt::UAuraFirebolt()
{
	SpellType = ESpellType::Projectile;
	MaxNumProjectiles = 5;
}

FString UAuraFirebolt::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	if (Level == 1)
	{
		return FString::Printf(TEXT(
			"<Title>파이어볼트</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			ScaledDamage + MagicPowerDamage
			);
	}
	else
	{
		return FString::Printf(TEXT(
			"<Title>파이어볼트</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 %d개 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
			Level,
			ManaCost,
			Cooldown,
			FMath::Min(MaxNumProjectiles, Level),
			ScaledDamage + MagicPowerDamage
		);
	}
}

FString UAuraFirebolt::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();

	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>적중하면 폭발하는 구체를 %d개 발사하여 </><Damage>%d</><Default>의 피해를 입히고 일정 확률로 대상에게 화상을 입힙니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		FMath::Min(MaxNumProjectiles, Level),
		ScaledDamage + MagicPowerDamage
	);
}

void UAuraFirebolt::SpawnProjectiles(const FVector& ProjectileTargetLocation, const FGameplayTag& SocketTag, bool bOverridePitch, float PitchOverride, bool bNumProjectileOverride, int32 NumProjectileOverride, AActor* HomingTarget)
{
	if (!CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}
	
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
	const int32 AbilityLevel = GetAbilityLevel();
	int32 EffectiveNumProjectiles = 0;

	if (!bNumProjectileOverride)
	{
		EffectiveNumProjectiles = FMath::Min(MaxNumProjectiles, NumProjectiles + FMath::DivideAndRoundDown(AbilityLevel, 2));
	}
	else
	{
		EffectiveNumProjectiles = FMath::Min(MaxNumProjectiles, NumProjectileOverride);
	}
	
	TArray<FRotator> Rotators = UAuraAbilitySystemLibrary::EvenlySpacedRotators(Forward, FVector::UpVector, ProjectileSpread, EffectiveNumProjectiles);

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
		
		float Distance = 0.f;
		
		// 투사체 유도
		// 대상이 몬스터인지 확인
		// 사거리 체크
		if (HomingTarget && HomingTarget->Implements<UCombatInterface>())
		{
			Distance = FVector::Dist(HomingTarget->GetActorLocation(), SocketLocation);
			if (Distance <= AbilityRange)
			{
				Projectile->ProjectileMovement->HomingTargetComponent = HomingTarget->GetRootComponent();
			}
			else
			{
				FVector NormalVector = (ProjectileTargetLocation - SocketLocation);
				NormalVector.Normalize();
				
				FVector NewVector = SocketLocation + (NormalVector * AbilityRange);

				FHitResult HitResult;

				GetWorld()->LineTraceSingleByChannel(HitResult, NewVector, NewVector + FVector(0,0,-300.f), ECollisionChannel::ECC_Visibility);
				
				Projectile->HomingTargetSceneComponent = NewObject<USceneComponent>(USceneComponent::StaticClass());
				Projectile->HomingTargetSceneComponent->SetWorldLocation(HitResult.Location);
				Projectile->ProjectileMovement->HomingTargetComponent = Projectile->HomingTargetSceneComponent;
			}
		}
		else // 대상이 몬스터가 아닌 대상
		{
			// 마우스 클릭 위치의 컴포넌트 가져오기
			// 새로운 씬 컴포넌트 생성 <- 가비지 컬렉터에 추가하기 위해 멤버 변수로 추가!!
			if (FVector::Distance(ProjectileTargetLocation, SocketLocation) <= AbilityRange)
			{
				Projectile->HomingTargetSceneComponent = NewObject<USceneComponent>(USceneComponent::StaticClass());
				Projectile->HomingTargetSceneComponent->SetWorldLocation(ProjectileTargetLocation);
				Projectile->ProjectileMovement->HomingTargetComponent = Projectile->HomingTargetSceneComponent;
			}
			else
			{
				FVector NormalVector = (ProjectileTargetLocation - SocketLocation);
				NormalVector.Normalize();
				
				FVector NewVector = SocketLocation + (NormalVector * AbilityRange);
				
				FHitResult HitResult;

				GetWorld()->LineTraceSingleByChannel(HitResult, NewVector, NewVector + FVector(0,0,-300.f), ECollisionChannel::ECC_Visibility);
				
				Projectile->HomingTargetSceneComponent = NewObject<USceneComponent>(USceneComponent::StaticClass());
				Projectile->HomingTargetSceneComponent->SetWorldLocation(HitResult.Location);
				Projectile->ProjectileMovement->HomingTargetComponent = Projectile->HomingTargetSceneComponent;
			}
		}
		// 유도 가속력
		Projectile->ProjectileMovement->HomingAccelerationMagnitude = FMath::FRandRange(HomingAccMin, HomingAccMax);
		Projectile->ProjectileMovement->InitialSpeed += SpeedUpCount * 100.f; 
		Projectile->ProjectileMovement->MaxSpeed += SpeedUpCount * 100.f; 
		Projectile->ProjectileMovement->bIsHomingProjectile = bLaunchHomingProjectile;

		Projectile->FinishSpawning(SpawnTransform);  
	}
}

void UAuraFirebolt::CheckAbilityUpgrades()
{
	const auto& Tags = FAuraGameplayTags::Get();

	// 업그레이드 태그 검증
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), Tags.Upgrades_Fire_FireBolt_IncreaseNum))
	{
		// 투사체 갯수 증가
		int32 StackCount = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), Tags.Upgrades_Fire_FireBolt_IncreaseNum);
		NumProjectiles += StackCount;
	}
	
	// 투사체 속도 증가
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), FGameplayTag::RequestGameplayTag("Upgrades.Fire.FireBolt.SpeedUp")))
	{
		SpeedUpCount = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), FGameplayTag::RequestGameplayTag("Upgrades.Fire.FireBolt.SpeedUp"));
	}
}
