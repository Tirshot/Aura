// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ArcaneShards.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Actor/PointCollection.h"
#include "Character/AuraCharacter.h"
#include "Interaction/PlayerInterface.h"
#include "Kismet/KismetMathLibrary.h"
#include "Player/AuraPlayerController.h"

FString UArcaneShards::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	return FString::Printf(TEXT(
		"<Title>아케인 파편</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>지정한 범위 중심에 최대 </><Damage>%d</><Default>의 피해를 입히는 아케인 파편 기둥을 </><Num>%d개</> 소환합니다.\n<Small>데미지는 대상과 각 기둥 사이의 거리에 비례합니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		ScaledDamage + MagicPowerDamage,
		Level
	);
}

FString UArcaneShards::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const int32 ScaledDamage = Damage.GetValueAtLevel(Level);
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);
	const float MagicAttackPower = UAuraAbilitySystemLibrary::GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MagicAttackPower);
	const int32 MagicPowerDamage = MagicAttackPower * MagicPowerCoefficient.GetValue();
	
	return FString::Printf(TEXT(
		"<Title>다음 레벨: </>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>지정한 범위 중심에 최대 </><Damage>%d</><Default>의 피해를 입히는 아케인 파편 기둥을 </><Num>%d개</> 소환합니다.\n<Small>데미지는 대상과 각 기둥 사이의 거리에 비례합니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		ScaledDamage + MagicPowerDamage,
		Level
	);
}

void UArcaneShards::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	HideMagicCircleAndRangeIndicator();
	
	// 데미지 입힘 종료
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShardSpawnTimer);
	}

	// 포인트 컬렉션이 아직도 파괴되지 않았으면 파괴
	if (IsValid(PointCollection))
		PointCollection->Destroy();
}

void UArcaneShards::CheckAbilityUpgrades()
{
	const auto& Tags = FAuraGameplayTags::Get();

	// 업그레이드 태그 검증
	// (1) 투사체 갯수 증가 태그
	FGameplayTag IncreaseNum = Tags.Upgrades_Arcane_ArcaneShards_IncreaseNum;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), IncreaseNum))
	{
		// 투사체 갯수 증가
		AdditionalShards = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), IncreaseNum);
		NumPoints += AdditionalShards;
	}

	FGameplayTag FirstLargeShard = Tags.Upgrades_Arcane_ArcaneShards_FirstLargeShard;
	if (HasUpgradeTag(GetAvatarActorFromActorInfo(), FirstLargeShard))
	{
		// 첫번째 기둥 크기 증가
		bIsFirstShardLarge = true;
	}
}

void UArcaneShards::ReceivedMouseHitResult(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!TargetDataHandle.IsValid(0))
		return;

	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0);
	const FHitResult* HitResult = TargetData->GetHitResult();

	// 마우스 히트 정보를 멤버 변수로 만듬
	CurrentTargetLocation = HitResult->ImpactPoint;

	if (AActor* AvatarActor = GetAvatarActorFromActorInfo())
	{
		if (APawn* AvatarPawn = Cast<APawn>(AvatarActor))
		{
			if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(AvatarPawn->GetController()))
			{
				CurrentTargetLocation = AuraPC->GetMagicCircleLocation();
			}
		}
	}
}

void UArcaneShards::CreatePointCollection()
{
	NumPoints = GetAbilityLevel();

	GroundPoints.Empty();
	
	FTransform PointCollectionTransform;
	PointCollectionTransform.SetLocation(CurrentTargetLocation);

	PointCollection = GetWorld()->SpawnActorDeferred<APointCollection>(
		PointCollectionClass,
		PointCollectionTransform,
		GetAvatarActorFromActorInfo(),
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	PointCollection->FinishSpawning(PointCollectionTransform);

	GroundPoints = PointCollection->GetGroundPoints(FMath::Min(NumPoints, MaxNumShards));
}

void UArcaneShards::ReadyToSpawnShards()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();

	// 범위 표시기 제거
	if (AvatarActor->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_HideMagicCircle(AvatarActor);
	}

	// UI 범위기 메시지 제거
	RemoveRangeSpellHelpMessage(AvatarActor);
	
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ShardSpawnTimer, this, &UArcaneShards::SpawnShards,SpawnShardsDeltaTime, true);
	}

	// 쿨다운, 코스트 적용
	CommitAbility(GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo());

	StopAutoRun();
}

void UArcaneShards::SpawnShards()
{
	if (Idx < GroundPoints.Num())
	{
		// 파편 위치 계산
		ShardSpawnLocation = GroundPoints[Idx]->GetComponentTransform().GetLocation();
		ShardSpawnRotation = GroundPoints[Idx]->GetComponentTransform().GetRotation().Rotator();
		
		SpawnCueAndApplyDamage();
		
		Idx++;
	}
	else
		EndSpawnShards();
}

void UArcaneShards::SpawnCueAndApplyDamage()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	if (Idx < GroundPoints.Num())
	{
		// 파편 위치 계산
		ShardSpawnLocation = GroundPoints[Idx]->GetComponentTransform().GetLocation();
		ShardSpawnRotation = GroundPoints[Idx]->GetComponentTransform().GetRotation().Rotator();

		TArray<AActor*> OutOverlappingActors;
		OutOverlappingActors.Empty();
		
		FGameplayCueParameters CueParams;

		if (bIsFirstShardLarge && Idx == 0)
		{
			int32 Stacks = GetUpgradeStackCount(AvatarActor, FAuraGameplayTags::Get().Upgrades_Arcane_ArcaneShards_FirstLargeShard);
			float SizeMultiplier = Stacks * UpgradeFirstShardSizeMultiplier;
			
			// 아케인 파편 기둥 이펙트 생성
			CueParams.Location = ShardSpawnLocation;
			CueParams.Normal = UKismetMathLibrary::GetRightVector(ShardSpawnRotation);
			CueParams.RawMagnitude = SizeMultiplier / 100.f;

			K2_ExecuteGameplayCueWithParams(FGameplayTag::RequestGameplayTag("GameplayCue.ArcaneShards"), CueParams);

			ApplyRadialDamage(OutOverlappingActors, RadialDamageOuterRadius + SizeMultiplier);
		}
		else
		{
			// 아케인 파편 기둥 이펙트 생성
			CueParams.Location = ShardSpawnLocation;
			CueParams.Normal = UKismetMathLibrary::GetRightVector(ShardSpawnRotation);
			CueParams.RawMagnitude = 1.f;
		
			K2_ExecuteGameplayCueWithParams(FGameplayTag::RequestGameplayTag("GameplayCue.ArcaneShards"), CueParams);
		
			ApplyRadialDamage(OutOverlappingActors, RadialDamageOuterRadius);
		}
	}
}

void UArcaneShards::EndSpawnShards()
{
	PointCollection->Destroy();

	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true, false);
}

void UArcaneShards::ApplyRadialDamage(TArray<AActor*>& OutOverlappingActors, float OuterRadius)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Empty();
	ActorsToIgnore.Add(AvatarActor);

	// 데미지 입힐 플레이어 계산
	UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(
		AvatarActor,
		OutOverlappingActors,
		ActorsToIgnore,
		OuterRadius,
		ShardSpawnLocation);

	for (AActor* DamagedActor : OutOverlappingActors)
	{
		if (!IsValid(DamagedActor))
			continue;

		// 아군이라면 건너뛰기
		if (!UAuraAbilitySystemLibrary::IsNotFriend(DamagedActor, AvatarActor))
		{
			continue;
		}

		// 클래스 디폴트를 이용하여 데미지 이펙트 파라미터 생성
		FVector DirectionOverride = DamagedActor->GetActorLocation() - ShardSpawnLocation;
		FDamageEffectParams Params = MakeDamageEffectParamsFromClassDefaults(
			DamagedActor,
			ShardSpawnLocation,
			true,
			DirectionOverride,
			true,
			DirectionOverride,
			true,
			45.f);

		// 데미지 적용
		UAuraAbilitySystemLibrary::ApplyDamageEffect(Params);
	}
}
