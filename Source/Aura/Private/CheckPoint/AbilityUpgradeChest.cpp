// Fill out your copyright notice in the Description page of Project Settings.


#include "CheckPoint/AbilityUpgradeChest.h"

#include "Game/AuraGameInstance.h"
#include "Game/LoadScreenSaveGame.h"
#include "Interaction/PlayerInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

AAbilityUpgradeChest::AAbilityUpgradeChest(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{

}

void AAbilityUpgradeChest::LoadActor_Implementation()
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
		Destroy();
}

void AAbilityUpgradeChest::BeginPlay()
{
	Super::BeginPlay();

	// 이미 존재하면 파괴
	auto* AuraGI = Cast<UAuraGameInstance>(GetGameInstance());
	if (AuraGI)
	{
		ULoadScreenSaveGame* SaveGame = Cast<ULoadScreenSaveGame>(UGameplayStatics::LoadGameFromSlot(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex));
		if (SaveGame && SaveGame->OneTimeUseActors.Contains(Guid))
		{
			Destroy();
		}
	}
}

void AAbilityUpgradeChest::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (!Guid.IsValid())
	{
		Guid = FGuid::NewGuid();
	}
}

void AAbilityUpgradeChest::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                           UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->HasAuthority())
		return;
	
	if (OtherActor->Implements<UPlayerInterface>())
	{
		HandleGlowEffects(OtherActor);
		
		bReached = true;

		// 사용한 액터로 기록
		auto* AuraGI = Cast<UAuraGameInstance>(GetGameInstance());
		if (AuraGI)
		{
			ULoadScreenSaveGame* SaveGame = Cast<ULoadScreenSaveGame>(UGameplayStatics::LoadGameFromSlot(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex));
			if (SaveGame)
			{
				SaveGame->OneTimeUseActors.Add(Guid, true);
				UGameplayStatics::SaveGameToSlot(SaveGame, AuraGI->LoadSlotName, AuraGI->LoadSlotIndex);
			}
		}
	}
}
