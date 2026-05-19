// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckPoint/AbilityUpgradeChest.h"

#include "Character/AuraCharacter.h"
#include "Game/AuraGameInstance.h"
#include "Game/AuraGameModeBase.h"
#include "Game/LoadScreenSaveGame.h"
#include "Interaction/PlayerInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerController.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

AAbilityUpgradeChest::AAbilityUpgradeChest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bReplicates = true;
}

void AAbilityUpgradeChest::PostNetInit()
{
	Super::PostNetInit();
	
	if (bReached)
		OnRep_Reached();
}

void AAbilityUpgradeChest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void AAbilityUpgradeChest::LoadActor_Implementation()
{
	if (!HasAuthority())
		return;
	
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
        
				// 서버도 직접 처리
				if (bReached)
					OnRep_Reached();
			}
		}
	}
}

FGuid AAbilityUpgradeChest::GetGuid_Implementation()
{
	return Guid;
}

bool AAbilityUpgradeChest::IsReached_Implementation()
{
	return bReached;
}

void AAbilityUpgradeChest::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			AuraGM->SaveOneTimeUseActor(Guid, bReached);
				
			if (AuraGM->IsOneTimeUseActorUsed(Guid))
			{
				bReached = true;
				ForceNetUpdate();
				
			}
		}
	}
	
	if (bReached)
		OnRep_Reached();
}

void AAbilityUpgradeChest::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (!Guid.IsValid())
	{
		Guid = FGuid::NewGuid();
	}
}

void AAbilityUpgradeChest::OnRep_Reached()
{
	Super::OnRep_Reached();
	
	if (bReached)
	{
		CheckPointReached(DynamicMI, nullptr);
	}
}

void AAbilityUpgradeChest::UpdateChestState()
{
	if (bReached)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		SetActorTickEnabled(false);
	}
}

void AAbilityUpgradeChest::OnTimelineAnimationFinished(AActor* InteractedActor)
{
	if (HasAuthority())
	{
		// 서버 - 맵 상태 저장
		if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			if (AAuraCharacter* Aura = Cast<AAuraCharacter>(InteractedActor))
			{
				if  (AAuraPlayerController* AuraPC = Aura->GetController<AAuraPlayerController>())
				{
					AuraPC->Server_CreateCardSelection(InteractedActor);
					AuraGM->SaveOneTimeUseActor(Guid, true);
				}
			}
		}
	}
	UpdateChestState();
}

#if WITH_EDITOR
void AAbilityUpgradeChest::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	// 액터가 배치될 때 호출되는 함수
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (!Guid.IsValid())
	{
		Guid = FGuid::NewGuid();
	}
}
#endif

void AAbilityUpgradeChest::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority())
		return;
	
	if (OtherActor->Implements<UPlayerInterface>())
	{
		bReached = true;
		
		CheckPointReached(DynamicMI, OtherActor);
	}
}
