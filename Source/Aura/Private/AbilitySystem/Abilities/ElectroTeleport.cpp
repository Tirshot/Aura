// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/ElectroTeleport.h"

#include "NavigationSystem.h"
#include "Character/AuraCharacter.h"
#include "Components/CapsuleComponent.h"

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

bool UElectroTeleport::TeleportToLocation(const FHitResult& HitResult)
{
	bool bSuccessfulTeleport = false;
	
	FVector InitialLocation = GetAvatarActorFromActorInfo()->GetActorLocation();
	FVector MouseHitLocation = HitResult.Location;
	double Distance = FMath::Abs((InitialLocation-MouseHitLocation).Length());

	float InitialZ = InitialLocation.Z;
	float MouseHitZ = MouseHitLocation.Z;
	float Height = MouseHitZ - InitialZ;

	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(GetAvatarActorFromActorInfo());
	float CapsuleHalfHeight = AuraCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	
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
		DestinationLocation.Location.Z += CapsuleHalfHeight;
		GetAvatarActorFromActorInfo()->SetActorLocation(DestinationLocation,false,nullptr,ETeleportType::None);
		bSuccessfulTeleport = true;
	}
	// Nav Mesh의 밖이라면 텔레포트 실패
	return bSuccessfulTeleport;
}
