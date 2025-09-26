// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraEnemySpawnVolume.h"

#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerController.h"

AAuraEnemySpawnVolume::AAuraEnemySpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	Box = CreateDefaultSubobject<UBoxComponent>("Box");
	SetRootComponent(Box);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionObjectType(ECC_WorldStatic);
	Box->SetCollisionResponseToChannels(ECR_Ignore);
	Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	Niagara = CreateDefaultSubobject<UNiagaraComponent>("Niagara");
	Niagara->SetupAttachment(GetRootComponent());
	Niagara->Activate();
}

void AAuraEnemySpawnVolume::LoadActor_Implementation()
{
	if (bReached)
	{
		Destroy();
	}
}

void AAuraEnemySpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	if (bShowArrow)
		Niagara->Activate();
	else
		Niagara->Deactivate();
	
	
	Box->OnComponentBeginOverlap.AddDynamic(this, &AAuraEnemySpawnVolume::OnSphereOverlap);
}

void AAuraEnemySpawnVolume::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 부딫힌 액터가 플레이어가 아니면 리턴
	if (OtherActor->Implements<UPlayerInterface>() == false)
		return;
	
	bReached = true;

	for (AAuraEnemySpawnPoint* Point : SpawnPoints)
	{
		if (IsValid(Point))
		{
			Point->SpawnEnemy();
		}
	}

	Box->SetVisibility(false);

	if (IsValid(Niagara))
		Niagara->DestroyComponent();

	// 오토 런
	if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(OtherActor))
	{
		AuraPC->SetCachedDestination(GetActorLocation());
	}
	
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
