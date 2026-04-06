// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraFireBlast.h"

#include "AuraGameplayTags.h"
#include "Actor/AuraFireBall.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

UAuraFireBlast::UAuraFireBlast()
{
	SpellType = ESpellType::Projectile;
	MaxNumProjectiles = 18;
}

void UAuraFireBlast::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAuraFireBlast::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FString UAuraFireBlast::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	return FString::Printf(TEXT(
		"<Title>화염 폭발</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>지정한 범위 내에 </><ManaCost>%d</>개 화염구를 방출하여 <Damage>%d</>의 방사형 피해를 입히고 폭발시킵니다.\n일정 확률로 적에게 화상을 입힙니다."),
		Level,
		ManaCost,
		Cooldown,
		NumFireBalls + Level,
		ScaledDamage + MagicPowerDamage
	);
} 

FString UAuraFireBlast::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>지정한 범위 내에 </><ManaCost>%d</>개 화염구를 방출하여 <Damage>%d</>의 방사형 피해를 입히고 폭발시킵니다.\n일정 확률로 적에게 화상을 입힙니다."),
		Level,
		ManaCost,
		Cooldown,
		NumFireBalls + Level,
		ScaledDamage + MagicPowerDamage
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
			GetAvatarActorFromActorInfo()->GetInstigatorController()->GetPawn(),
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		
		FireBall->SetOwner(GetAvatarActorFromActorInfo());

		// 클래스 디폴트의 데미지 값 적용
		FireBall->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
		FireBall->ReturnToActor = GetAvatarActorFromActorInfo();
		FireBall->ExplosionDamageParams = MakeDamageEffectParamsFromClassDefaults();

		FireBalls.Add(FireBall);

		FireBall->FinishSpawning(SpawnTransform);
	}
	return FireBalls;
}

void UAuraFireBlast::CheckAbilityUpgrades()
{
	NumFireBalls = BaseNumFireBalls + GetAbilityLevel();
	
	const auto& Tags = FAuraGameplayTags::Get();
	
	// (1) 투사체 갯수 증가 체크
	FGameplayTag IncreaseNum = Tags.Upgrades_Fire_FireBlast_IncreaseNum;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseNum))
	{
		// 투사체 갯수 증가
		int32 StackCount = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncreaseNum);
		int32 Magnification = 2;

		// 갯수 2개 씩 증가
		NumFireBalls = BaseNumFireBalls + (StackCount * Magnification) + GetAbilityLevel();
	}
}