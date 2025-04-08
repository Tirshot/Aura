// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckPoint/CheckPoint.h"
#include "Components/SphereComponent.h"
#include "Interaction/PlayerInterface.h"

ACheckPoint::ACheckPoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	CheckpointMesh = CreateDefaultSubobject<UStaticMeshComponent>("CheckpointMesh");
	CheckpointMesh->SetupAttachment(GetRootComponent());
	CheckpointMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CheckpointMesh->SetCollisionResponseToAllChannels(ECR_Block);

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	Sphere->SetupAttachment(CheckpointMesh);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void ACheckPoint::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_SaveProgress(OtherActor, PlayerStartTag);
		HandleGlowEffects();
	}
}

void ACheckPoint::BeginPlay()
{
	Super::BeginPlay();

	Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACheckPoint::OnSphereOverlap);
}

void ACheckPoint::HandleGlowEffects()
{
	// 콜리전 끄기
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 동적 머티리얼 인스턴스 생성
	UMaterialInstanceDynamic* DynamicMI = UMaterialInstanceDynamic::Create(CheckpointMesh->GetMaterial(0), this);

	// 머티리얼을 동적 머티리얼 인스턴스로 설정
	CheckpointMesh->SetMaterial(0, DynamicMI);

	// 블루프린트 함수 호출
	CheckPointReached(DynamicMI);
}
