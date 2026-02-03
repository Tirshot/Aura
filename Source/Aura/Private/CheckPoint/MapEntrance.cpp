// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckPoint/MapEntrance.h"

#include "Components/SphereComponent.h"
#include "Game/AuraGameModeBase.h"
#include "Interaction/PlayerInterface.h"
#include "Kismet/GameplayStatics.h"

AMapEntrance::AMapEntrance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Sphere->SetupAttachment(MoveToComponent);
}

void AMapEntrance::HighlightActor_Implementation()
{
	// bReached 여부에 상관 없이 하이라이트
	CheckpointMesh->SetRenderCustomDepth(true);
}

void AMapEntrance::LoadActor_Implementation()
{
	// 아무 것도 하지 않음
}

void AMapEntrance::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->Implements<UPlayerInterface>())
	{
		bReached = true;
		
		// 캐릭터 저장
		IPlayerInterface::Execute_SaveProgress(OtherActor, PlayerStartTag);
		
		// 맵 상태 저장
		Server_SaveWorldState();
		
		// 맵 이동
		Server_TravelToNextMap();
	}
}

void AMapEntrance::Server_TravelToNextMap_Implementation()
{
	if (HasAuthority())
	{
		GetWorld()->ServerTravel(DestinationMap.ToSoftObjectPath().GetAssetName());
	}
}

void AMapEntrance::Server_SaveWorldState_Implementation()
{
	if (HasAuthority())
	{
		if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			// 목적지 맵의 상태를 저장
			AuraGM->Server_SaveWorldState(GetWorld(), DestinationMap.ToSoftObjectPath().GetAssetName());
		}
	}
}
