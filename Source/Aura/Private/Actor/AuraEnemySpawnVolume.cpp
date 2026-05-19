// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AuraEnemySpawnVolume.h"

#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Game/AuraGameInstance.h"
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerController.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

AAuraEnemySpawnVolume::AAuraEnemySpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
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
	if (UAuraGameInstance* AuraGI = Cast<UAuraGameInstance>(GetGameInstance()))
	{
		if (auto* SaveObject = AuraGI->GetSaveSlotData(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex))
		{
			FString WorldName = GetWorld()->GetMapName();
			WorldName.RemoveFromStart(GetWorld()->StreamingLevelsPrefix);
        	
			// 저장된 데이터 가져오기
			const FSavedMap& SavedMap = SaveObject->GetSavedMapWithMapName(WorldName);
			
			// 찾기 시작
			const FSavedActor* FoundSavedActor = nullptr;
			const FName Name = GetFName();

			for (const FSavedActor& SavedActorData : SavedMap.SavedActors)
			{
				if (SavedActorData.ActorName == Name)
				{
					FoundSavedActor = &SavedActorData;
					break;
				}
			}
        		
			if (FoundSavedActor && FoundSavedActor->Bytes.Num() > 0)
			{
				FMemoryReader MemoryReader(FoundSavedActor->Bytes);
				FObjectAndNameAsStringProxyArchive Archive(MemoryReader, true);
				Archive.ArIsSaveGame = true;

				this->Serialize(Archive); 
				ForceNetUpdate();
			}
		}
	}
	
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

	Box->SetVisibility(false);

	if (IsValid(Niagara))
		Niagara->DestroyComponent();

	// 오토 런
	if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(OtherActor))
	{
		AuraPC->StopAutoRun();
	}
	
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 이중 스폰 방지 - 서버에서만 소환
	if (OtherActor->HasAuthority())
	{
		for (AAuraEnemySpawnPoint* Point : SpawnPoints)
		{
			if (IsValid(Point))
			{
				Point->SpawnEnemy();
			}
		}
	}
}
