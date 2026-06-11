// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/AuraSpawnElectroSphere.h"

#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Actor/AuraElectroSphere.h"
#include "GameFramework/ProjectileMovementComponent.h"

FString UAuraSpawnElectroSphere::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();

	const float RealDamage = ScaledDamage + MagicPowerDamage;
	
	return FString::Printf(TEXT(
		"<Title>전기 구체 소환</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>지정한 범위에 </><Num>%.1f</><Default>초 동안 매 </><Num>%.1f</><Default>초 마다 주위 </><Num>%d</><Default>명의 대상에게 </><Damage>%.1f</><Default>의 피해를 입히는 전기 구체를 소환합니다.</>\n<Small>전기 구체는 </><Num>%1.f</><Small>의 속도로 움직이며, 물체와 접촉하면 반사됩니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		AbilityRange,
		SpawnTime,
		DamageDeltaSecond,
		NumAdditionalTargets,
		RealDamage,
		MovementSpeed
	);
}

FString UAuraSpawnElectroSphere::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();

	const float RealDamage = ScaledDamage + MagicPowerDamage;
	
	return FString::Printf(TEXT(
	"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Small>범위 </><Range>%.1f</>\n<Default>지정한 범위에 </><Num>%.1f</><Default>초 동안 매 </><Num>%.1f</><Default>초 마다 주위 </><Num>%d</><Default>명의 대상에게 </><Damage>%.1f</><Default>의 피해를 입히는 전기 구체를 소환합니다.</>\n<Small>전기 구체는 </><Num>%1.f</><Small>의 속도로 움직이며, 물체와 접촉하면 반사됩니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		AbilityRange,
		SpawnTime,
		DamageDeltaSecond,
		NumAdditionalTargets,
		RealDamage,
		MovementSpeed
	);
}

void UAuraSpawnElectroSphere::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UAuraSpawnElectroSphere::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UAuraSpawnElectroSphere::CheckAbilityUpgrades()
{
	const auto& Tags = FAuraGameplayTags::Get();

	// 업그레이드 태그 검증
	// (1) 추적 범위 증가 
	FGameplayTag IncreaseRange = Tags.Upgrades_Lightning_SpawnElectroSphere_IncreaseTraceRange;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseRange))
	{
		int Stacks = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncreaseRange);
	
		TraceRadius += 100.f * Stacks;
	}

	// (2) 이동속도 감소
	FGameplayTag DecreaseSpeed = Tags.Upgrades_Lightning_SpawnElectroSphere_DecreaseMovementSpeed;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), DecreaseSpeed))
	{
		int Stacks = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), DecreaseSpeed);
	
		ElectroSphere->AddMovementSpeed(-125.f * Stacks);
	}

	// (3) 가까운 대상 유도
	FGameplayTag HomingNearestTarget = Tags.Upgrades_Lightning_SpawnElectroSphere_HomingNearestTarget;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), HomingNearestTarget))
	{
		ElectroSphere->SetHomingTarget(true);
	}

	// (4) 소환 자리 정지
	FGameplayTag StopMovement = FGameplayTag::RequestGameplayTag("Upgrades.Lightning.SpawnElectroSphere.StopMovement");
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), StopMovement))
	{
		ElectroSphere->SetMovementSpeed(0.f);
		ElectroSphere->ProjectileMovement->Velocity = FVector::ZeroVector;
		ElectroSphere->ProjectileMovement->StopSimulating(FHitResult()); 
		ElectroSphere->ProjectileMovement->UpdateComponentVelocity();
	}
}

AAuraElectroSphere* UAuraSpawnElectroSphere::SpawnElectroSphere(const FVector& Location)
{
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(Location);
	
	FVector RelativeVector = Location - GetAvatarActorFromActorInfo()->GetActorLocation();
	RelativeVector.Normalize();

	AAuraElectroSphere* Sphere = GetWorld()->SpawnActorDeferred<AAuraElectroSphere>(
		ElectroSphereClass,
		SpawnTransform,
		GetOwningActorFromActorInfo(),
		CurrentActorInfo->PlayerController->GetPawn(),
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	// 바닥에서 크기만큼 띄우기
	const float AdditionalHeight = Sphere->GetSphereRadius() * 2;
	Sphere->SetActorLocation(Location + FVector(0.f, 0.f, AdditionalHeight));

	// 진행 속도 설정
	Sphere->ProjectileMovement->SetVelocityInLocalSpace(FVector(RelativeVector.X * MovementSpeed, RelativeVector.Y * MovementSpeed, 0));
	
	// 파라미터 설정
	Sphere->SetDamageDeltaSecond(DamageDeltaSecond);
	Sphere->SetMovementSpeed(MovementSpeed);
	Sphere->SetTraceRadius(TraceRadius);
	Sphere->SetDamageRadius(DamageRadius);
	Sphere->SetFollowToTarget(bFollowTarget);

	// 데미지 타겟 설정
	Sphere->SetNumAdditionalTargets(NumAdditionalTargets);
	
	Sphere->DamageEffectParams = MakeDamageEffectParamsFromClassDefaults();
	Sphere->SetOwner(GetAvatarActorFromActorInfo());

	Sphere->FinishSpawning(SpawnTransform);
	Sphere->SetLifeSpan(SpawnTime);
	
	ElectroSphere = Sphere;
	
	return Sphere;
}