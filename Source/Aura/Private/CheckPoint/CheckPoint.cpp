// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckPoint/CheckPoint.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Aura/Aura.h"
#include "Character/AuraCharacter.h"
#include "Components/SphereComponent.h"
#include "Game/AuraGameModeBase.h"
#include "Interaction/PlayerInterface.h"
#include "Kismet/GameplayStatics.h"

ACheckPoint::ACheckPoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	CheckpointMesh = CreateDefaultSubobject<UStaticMeshComponent>("CheckpointMesh");
	CheckpointMesh->SetupAttachment(GetRootComponent());
	CheckpointMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CheckpointMesh->SetCollisionResponseToAllChannels(ECR_Block);
	CheckpointMesh->SetCustomDepthStencilValue(CustomDepthStencilOverride);
	CheckpointMesh->MarkRenderStateDirty();

	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	Sphere->SetupAttachment(CheckpointMesh);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// MoveToLocation의 목적지
	MoveToComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MoveToComponent"));
	MoveToComponent->SetupAttachment(GetRootComponent());
}

void ACheckPoint::HighlightActor_Implementation()
{
	if (bReached == false)
		CheckpointMesh->SetRenderCustomDepth(true);
}

void ACheckPoint::UnHighlightActor_Implementation()
{
	CheckpointMesh->SetRenderCustomDepth(false);
}

void ACheckPoint::SetMoveToLocation_Implementation(FVector& OutDestination)
{
	OutDestination = MoveToComponent->GetComponentLocation();
}

void ACheckPoint::LoadActor_Implementation()
{
	// 이미 도달한 적 있으면 이펙트 활성화
	if (bReached)
	{
		HandleGlowEffects();
	}
}

void ACheckPoint::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (OtherActor->Implements<UPlayerInterface>())
	{
		bReached = true;

		// 월드 상태 저장
		if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			const UWorld* World = GetWorld();
			FString MapName = World->GetMapName();
			MapName.RemoveFromStart(World->StreamingLevelsPrefix);
			
			AuraGM->SaveWorldState(GetWorld(), MapName);
		}
		
		IPlayerInterface::Execute_SaveProgress(OtherActor, PlayerStartTag);

		if (AAuraCharacter* AvatarActor = Cast<AAuraCharacter>(OtherActor))
		{
			if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AvatarActor->GetAbilitySystemComponent()))
			{
				FGameplayEffectContextHandle ContextHandle = AuraASC->MakeEffectContext();
				ContextHandle.AddSourceObject(AvatarActor);
				
				FGameplayEffectSpecHandle SpecHandle = AuraASC->MakeOutgoingSpec(AuraHeal, 1.f, ContextHandle);
				AuraASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
		HandleGlowEffects();
	}
}

void ACheckPoint::BeginPlay()
{
	Super::BeginPlay();

	if (bBindOverlapCallback)
		Sphere->OnComponentBeginOverlap.AddDynamic(this, &ACheckPoint::OnSphereOverlap);
}

void ACheckPoint::HandleGlowEffects()
{
	// 콜리전 끄기
	Sphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 동적 머티리얼 생성
	UMaterialInstanceDynamic* DynamicMI = UMaterialInstanceDynamic::Create(CheckpointMesh->GetMaterial(0), this);

	// 메시의 머티리얼을 동적 머티리얼로 지정
	CheckpointMesh->SetMaterial(0, DynamicMI);

	// 블루프린트로 전달
	CheckPointReached(DynamicMI);
}
