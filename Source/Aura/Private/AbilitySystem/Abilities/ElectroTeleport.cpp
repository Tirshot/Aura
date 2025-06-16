// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ElectroTeleport.h"

#include "AbilitySystemComponent.h"
#include "AuraGameplayTags.h"
#include "NavigationSystem.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Actor/GhostEffectActor.h"
#include "Character/AuraCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/KismetMathLibrary.h"

UElectroTeleport::UElectroTeleport()
{
	SpellType = ESpellType::NonTargeting;
}

FString UElectroTeleport::GetDescription(int32 Level, const UObject* WorldContextObject)
{
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Title>순간 이동</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>최대 </><Damage>%.f</><Default>의 거리를 뛰어넘어 이동합니다.</>\n<Default>레벨이 상승할수록 마나와 쿨타임이 감소합니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		MaxTeleportDistance
	);
}

FString UElectroTeleport::GetNextLevelDescription(int32 Level, const UObject* WorldContextObject)
{
	const float ManaCost = FMath::Abs(GetManaCost(Level));
	const float Cooldown = GetCoolDown(Level);

	return FString::Printf(TEXT(
		"<Title>다음 레벨 :</>\n<Small>레벨 </><Level>%d</>\n<Small>마나 </><ManaCost>%.1f</>\n<Small>쿨타임 </><Cooldown>%.1f</>\n<Default>최대 </><Damage>%.f</><Default>의 거리를 뛰어넘어 이동합니다.</>\n<Default>레벨이 상승할수록 마나와 쿨타임이 감소합니다.</>"),
		Level,
		ManaCost,
		Cooldown,
		MaxTeleportDistance
	);
}

bool UElectroTeleport::CheckAbilityUpgrades(FGameplayTag AbilityTag)
{
	bool bUpgradesApplied = false;
	
	TArray<FAuraAbilityUpgradeInfo> Upgrades = GetAbilityUpgradeForTag(GetAvatarActorFromActorInfo(), AbilityTag);
	if (Upgrades.IsEmpty())
		return false;
	
	const auto& Tags = FAuraGameplayTags::Get();

	for (const auto& Upgrade : Upgrades)
	{
		// (1) 텔레포트 쿨타임 초기화 업그레이드
		FGameplayTag CooldownResetTag = Tags.Upgrades_Lightning_Teleport_DecreaseCoolDown;
		if (HasUpgradeTag(GetAvatarActorFromActorInfo(), CooldownResetTag))
		{
			int32 StackCount = GetUpgradeStackCount(GetAvatarActorFromActorInfo(), CooldownResetTag);
			
			// 스택 당 확률 증가
			TeleportCooldownResetProbability = StackCount * 10.f;
			bUpgradesApplied = true;
		}
	}

	return bUpgradesApplied;
}

bool UElectroTeleport::TeleportToLocation(const FHitResult& HitResult)
{
	bool bSuccessfulTeleport = false;
	
	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(GetAvatarActorFromActorInfo());
	
	float CapsuleHalfHeight = AuraCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector InitialLocation = AuraCharacter->GetActorLocation() - FVector(0, 0, CapsuleHalfHeight); ;
	FVector MouseHitLocation = HitResult.Location;
	double Distance = FMath::Abs((InitialLocation-MouseHitLocation).Length());

	float InitialZ = InitialLocation.Z;
	float MouseHitZ = MouseHitLocation.Z;
	float Height = MouseHitZ - InitialZ;
	
	if (HitResult.bBlockingHit == false)
		return bSuccessfulTeleport;

	// 거리 계산
	if (Distance > MaxTeleportDistance || Height > MaxHeight)
		return bSuccessfulTeleport;
	
	// NavMesh 검사
	FNavLocation DestinationLocation;
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys && NavSys->ProjectPointToNavigation(MouseHitLocation, DestinationLocation, FVector(50.f, 50.f, 200.f)))
	{
		// DestinationLocation.Location.Z += CapsuleHalfHeight;
		// GetAvatarActorFromActorInfo()->SetActorLocation(DestinationLocation,false,nullptr,ETeleportType::None);

		// 텔레포트할 최종 목표 위치 (마우스 클릭 지상 + 캐릭터 캡슐 절반 높이)
		FVector FinalTeleportLocation = MouseHitLocation + FVector(0, 0, CapsuleHalfHeight);

		// 텔레포트 수행
		AuraCharacter->SetActorLocation(FinalTeleportLocation, false, nullptr, ETeleportType::TeleportPhysics);
		bSuccessfulTeleport = true;
	}
	// Nav Mesh의 밖이라면 텔레포트 실패
	return bSuccessfulTeleport;
}

void UElectroTeleport::GhostEffect(const FVector& InitialLocation, const FVector& DestinationLocation, TSubclassOf<AGhostEffectActor> GhostClass, UMaterialInterface* GhostMaterial)
{
	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(GetAvatarActorFromActorInfo());
	if (!AuraCharacter || NumGhosts <= 0)
		return;

	for (int i = 0; i < NumGhosts; i++)
	{
		float N;
		if (NumGhosts == 1)
			N = 0.5f;
		else
			N = (float)i / (NumGhosts - 1.0f);
			
		// 선형 보간
		FVector GhostLocation = FMath::Lerp(InitialLocation, DestinationLocation, N);
		FRotator GhostRotation = AuraCharacter->GetActorRotation();
		FTransform SpawnTransform(GhostRotation, GhostLocation);

		AGhostEffectActor* Ghost = GetWorld()->SpawnActorDeferred<AGhostEffectActor>(
			GhostClass,
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn
		);

		if (Ghost)
		{
			Ghost->SetGhostActorMesh(AuraCharacter->GetMesh(), GhostMaterial);
			Ghost->SetLifeSpan(GhostLifeSpan + (i * 0.2));
			Ghost->FinishSpawning(SpawnTransform);
		}
	}
}

bool UElectroTeleport::ShouldTeleportCooldownReset()
{
	int RandValue = UKismetMathLibrary::RandomIntegerInRange(1, 100);
	if (RandValue <= TeleportCooldownResetProbability)
		return true;

	return false;
}
