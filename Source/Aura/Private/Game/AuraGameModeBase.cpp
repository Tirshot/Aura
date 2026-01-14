// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameModeBase.h"

#include "AuraGameplayTags.h"
#include "AuraLogChannels.h"
#include "EngineUtils.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Actor/AuraDropItem.h"
#include "AI/AuraAIController.h"
#include "Character/AuraBossMonster.h"
#include "Character/AuraCharacter.h"
#include "Game/LoadScreenSaveGame.h"
#include "UI/ViewModel/MVVM_LoadSlot.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Game/AuraGameInstance.h"
#include "Interaction/SaveInterface.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "GameFramework/Character.h"
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"
#include "UI/WidgetController/GameOverWidgetController.h"

void AAuraGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	Maps.Add(DefaultMapName, DefaultMap);

	OnAllActorsInvincible.AddDynamic(this, &AAuraGameModeBase::SetAllActorsInvincible);
}

void AAuraGameModeBase::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer)
	{
		if (AAuraPlayerState* AuraPS = NewPlayer->GetPlayerState<AAuraPlayerState>())
		{
			AuraPS->OnPlayerStateInitialized.AddDynamic(this, &AAuraGameModeBase::HandlePlayerStateInitialized);
		}
	}
}

AActor* AAuraGameModeBase::ChoosePlayerStart_Implementation(AController* Player)
{
	// 게임 인스턴스로 데이터 넘김
	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());

	// 맵 내의 플레이어 스타트 가져오기
	TArray<AActor*> Actors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), Actors);

	if (Actors.Num() > 0)
	{
		// 첫 플레이어 스타트를 선택하고, 지정한 플레이어 스타트가 있다면 해당 스타트에서 시작
		AActor* SelectedActor = Actors[0];
		for (AActor* Actor : Actors)
		{
			if (APlayerStart* PlayerStart = Cast<APlayerStart>(Actor))
			{
				if (PlayerStart->PlayerStartTag == AuraGameInstance->PlayerStartTag && PlayerStart->PlayerStartTag != FName("None"))
				{
					SelectedActor = PlayerStart;
					break;
				}
			}
		}
		return SelectedActor;
	}
	return nullptr;
}

void AAuraGameModeBase::SaveSlotData(UMVVM_LoadSlot* LoadSlot, int32 SlotIndex)
{
	// 이미 게임이 존재하면 데이터 삭제
	if (UGameplayStatics::DoesSaveGameExist(LoadSlot->GetLoadSlotName(), SlotIndex))
	{
		UGameplayStatics::DeleteGameInSlot(LoadSlot->GetLoadSlotName(), SlotIndex);
	}

	// 저장 오브젝트 생성
	USaveGame* SaveGameObject = UGameplayStatics::CreateSaveGameObject(LoadScreenSaveGameClass);

	// 데이터를 집어넣기
	ULoadScreenSaveGame* LoadScreenSaveGame = Cast<ULoadScreenSaveGame>(SaveGameObject);
	LoadScreenSaveGame->PlayerName = LoadSlot->GetPlayerName();
	LoadScreenSaveGame->MapName = LoadSlot->GetMapName();
	LoadScreenSaveGame->SaveSlotStatus = Taken;
	LoadScreenSaveGame->PlayerStartTag = LoadSlot->PlayerStartTag;
	LoadScreenSaveGame->MapAssetName = LoadSlot->MapAssetName;

	// 최종 저장
	UGameplayStatics::SaveGameToSlot(LoadScreenSaveGame, LoadSlot->GetLoadSlotName(), SlotIndex);
}

void AAuraGameModeBase::DeleteSlot(const FString& SlotName, int32 SlotIndex)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, SlotIndex);
	}
}

ULoadScreenSaveGame* AAuraGameModeBase::RetrieveInGameSaveData()
{
	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());

	const FString InGameLoadSlotName = AuraGameInstance->LoadSlotName;
	const int32 InGameLoadSlotIndex = AuraGameInstance->LoadSlotIndex;

	return GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
}

void AAuraGameModeBase::SaveInGameProgressData(ULoadScreenSaveGame* SaveObject)
{
	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());

	const FString InGameLoadSlotName = AuraGameInstance->LoadSlotName;
	const int32 InGameLoadSlotIndex = AuraGameInstance->LoadSlotIndex;
	AuraGameInstance->PlayerStartTag = SaveObject->PlayerStartTag;

	UGameplayStatics::SaveGameToSlot(SaveObject, InGameLoadSlotName, InGameLoadSlotIndex);
}

void AAuraGameModeBase::SaveWorldState(UWorld* World, const FString& DestinationMapAssetName)
{
	// 접두사를 제외한 실제 이름만 가져오기
	FString WorldName = World->GetMapName();
	WorldName.RemoveFromStart(World->StreamingLevelsPrefix);

	UAuraGameInstance* AuraGI = Cast<UAuraGameInstance>(GetGameInstance());
	check(AuraGI);
	
	if (ULoadScreenSaveGame* SaveGame = GetSaveSlotData(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex))
	{
		// 맵의 이름 지정
		if (DestinationMapAssetName != FString(""))
		{
			SaveGame->MapAssetName = DestinationMapAssetName;
			SaveGame->MapName = GetMapNameFromMapAssetName(DestinationMapAssetName);
		}
		
		// 저장 데이터의 배열에 해당 맵이 없다면 추가
		if (SaveGame->HasMap(WorldName) == false)
		{
			FSavedMap NewSavedMap;
			NewSavedMap.MapAssetName = WorldName;
			SaveGame->SavedMaps.Add(NewSavedMap);
		}

		//
		FSavedMap SavedMap = SaveGame->GetSavedMapWithMapName(WorldName);
		SavedMap.SavedActors.Empty();

		// 월드 내의 모든 액터 순회
		// 최적화를 위해 특정 조건을 만들수도 있음
		for (FActorIterator It(World); It; ++It)
		{
			AActor* Actor = *It;

			// 사망 상태가 아닐 때 또는 저장 인터페이스를 상속받지 않을 때 패스
			if (IsValid(Actor) == false || Actor->Implements<USaveInterface>() == false)
				continue;

			FSavedActor SavedActor;
			SavedActor.ActorName = Actor->GetFName();
			SavedActor.Transform = Actor->GetTransform();

			// 메모리 라이터 생성
			FMemoryWriter MemoryWriter(SavedActor.Bytes);
			// 아카이브 생성
			FObjectAndNameAsStringProxyArchive Archive(MemoryWriter, true);
			Archive.ArIsSaveGame = true;

			// 직렬화
			Actor->Serialize(Archive);

			SavedMap.SavedActors.AddUnique(SavedActor);
		}

		// 이전 맵에 대한 정보 제거
		for (FSavedMap& MapToReplace : SaveGame->SavedMaps)
		{
			if (MapToReplace.MapAssetName == WorldName)
			{
				MapToReplace = SavedMap;
			}
		}
		// 데이터 저장
		UGameplayStatics::SaveGameToSlot(SaveGame, AuraGI->LoadSlotName, AuraGI->LoadSlotIndex);
	}
}

void AAuraGameModeBase::LoadWorldState(UWorld* World)
{
	// 접두사를 제외한 실제 이름만 가져오기
	FString WorldName = World->GetMapName();
	WorldName.RemoveFromStart(World->StreamingLevelsPrefix);

	UAuraGameInstance* AuraGI = Cast<UAuraGameInstance>(GetGameInstance());
	check(AuraGI);
	
	if (UGameplayStatics::DoesSaveGameExist(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex))
	{
		ULoadScreenSaveGame* SaveGame = Cast<ULoadScreenSaveGame>(UGameplayStatics::LoadGameFromSlot(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex));
		if (SaveGame == nullptr)
		{
			UE_LOG(LogAura, Error, TEXT("Failed to load slot"));
			return;
		}
		
		for (FActorIterator It(World); It; ++It)
		{
			AActor* Actor = *It;

			// 저장 인터페이스가 없는 액터는 스킵
			if (Actor->Implements<USaveInterface>() == false)
				continue;

			// 저장된 액터 배열 순회
			for (FSavedActor SavedActor : SaveGame->GetSavedMapWithMapName(WorldName).SavedActors)
			{
				// 저장된 액터가 순회 중인 월드의 액터와 일치
				if (SavedActor.ActorName == Actor->GetFName())
				{
					// 트랜스폼을 불러와야 하면 불러오기
					if (ISaveInterface::Execute_ShouldLoadTransform(Actor))
					{
						Actor->SetActorTransform(SavedActor.Transform);
					}

					// 역직렬화
					FMemoryReader MemoryReader(SavedActor.Bytes);
					FObjectAndNameAsStringProxyArchive Archive(MemoryReader, true);
					Archive.ArIsSaveGame = true;

					Actor->Serialize(Archive); // Bytes를 변수로 다시 변환

					// 액터의 불러오기 함수 호출
					ISaveInterface::Execute_LoadActor(Actor);
				}
			}
		}
	}
}

void AAuraGameModeBase::TravelToMap(UMVVM_LoadSlot* Slot)
{
	const FString SlotName = Slot->GetLoadSlotName();
	const int32 SlotIndex = Slot->SlotIndex;

	UGameplayStatics::OpenLevelBySoftObjectPtr(Slot, Maps.FindChecked(Slot->GetMapName()));
}

void AAuraGameModeBase::TravelToMap(FString MapName)
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Maps.FindChecked(MapName));
}

ULoadScreenSaveGame* AAuraGameModeBase::GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const
{
	USaveGame* SaveGameObject = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, SlotIndex))
	{
		// 데이터가 있으면 불러오기
		SaveGameObject = UGameplayStatics::LoadGameFromSlot(SlotName, SlotIndex);
	}
	else
	{
		// 데이터가 없으면 생성
		SaveGameObject = UGameplayStatics::CreateSaveGameObject(LoadScreenSaveGameClass);
	}

	// 커스텀 세이브로 캐스팅 후 리턴
	ULoadScreenSaveGame* LoadScreenSaveGame = Cast<ULoadScreenSaveGame>(SaveGameObject);
	return LoadScreenSaveGame;
}

void AAuraGameModeBase::GameAutoSave()
{
	// 월드 상태 저장
	if (AAuraGameModeBase* AuraGM = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		const UWorld* World = GetWorld();
		FString MapName = World->GetMapName();
		MapName.RemoveFromStart(World->StreamingLevelsPrefix);
			
		AuraGM->SaveWorldState(GetWorld(), MapName);

		auto* PlayerController = GetWorld()->GetFirstPlayerController();
		AActor* Actor = Cast<AActor>(PlayerController->GetPawn());
		if (Actor->Implements<UPlayerInterface>())
			IPlayerInterface::Execute_SaveProgress(Actor, "");
	}
}

FString AAuraGameModeBase::GetMapNameFromMapAssetName(const FString& MapAssetName)
{
	for (auto& Map : Maps)
	{
		if (Map.Value.ToSoftObjectPath().GetAssetName() == MapAssetName)
			return Map.Key;
	}
	return FString();
}

void AAuraGameModeBase::OnBossMonsterDead(AActor* DeadActor)
{
	// 월드 상태 저장
	GameAutoSave();
}

void AAuraGameModeBase::AddMonsterToArray(AAuraEnemy* Enemy)
{
	if (AAuraBossMonster* Boss = Cast<AAuraBossMonster>(Enemy))
	{
		BossCharacters.Add(Boss);
		OnBossMonsterCountChanged.Broadcast(BossCharacters.Num());

		// 보스 사망 델리게이트 구독 -> 게임 강제 저장
		Boss->OnDeath.AddDynamic(this, &AAuraGameModeBase::OnBossMonsterDead);

		if (AAuraPlayerController* AuraPC = GetWorld()->GetFirstPlayerController<AAuraPlayerController>())
		{
			AuraPC->OnBossMonsterAdded.Broadcast();
		}
	}
	else
	{
		EnemyCharacters.Add(Enemy);
		OnMonsterCountChanged.Broadcast(EnemyCharacters.Num());
	}
}

void AAuraGameModeBase::RemoveMonsterFromArray(AAuraEnemy* Enemy)
{
	if (AAuraBossMonster* Boss = Cast<AAuraBossMonster>(Enemy))
	{
		BossCharacters.Remove(Boss);
		OnBossMonsterCountChanged.Broadcast(BossCharacters.Num());
	}
	else
	{
		EnemyCharacters.Remove(Enemy);
		OnMonsterCountChanged.Broadcast(EnemyCharacters.Num());
	}
}

void AAuraGameModeBase::SetAllActorsInvincible(bool bInvincible)
{
	if (!HasAuthority())
		return;

	// 플레이어 순회
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(It->Get()))
		{
			AuraPC->Server_CharacterInvincible(bInvincible);
		}
	}
    
	// 일반 몬스터 순회
	for (auto MonsterPtr : EnemyCharacters) 
	{
		if (APawn* EnemyPawn = Cast<APawn>(MonsterPtr.Get()))
		{
			if (AAuraAIController* AuraAIController = EnemyPawn->GetController<AAuraAIController>())
			{
				AuraAIController->Server_CharacterInvincible(bInvincible);
			}
		}
	}

	// 보스 몬스터 순회
	for (auto BossPtr : BossCharacters)
	{
		if (APawn* BossPawn = Cast<APawn>(BossPtr.Get()))
		{
			if (AAuraAIController* AuraAIController = BossPawn->GetController<AAuraAIController>())
			{
				AuraAIController->Server_CharacterInvincible(bInvincible);
			}
		}
	}
}

void AAuraGameModeBase::PlayerDied(ACharacter* DeadCharacter, float RemainingTime)
{
	if (UGameOverWidgetController* GameOverWC = UAuraAbilitySystemLibrary::GetGameOverWidgetController(DeadCharacter))
	{
		GameOverWC->SetRemainingTime(RemainingTime);
	}
}

void AAuraGameModeBase::RestartGameFromSaveData(ACharacter* DeadCharacter)
{
	// 저장 오브젝트 불러와 마지막 저장 확인
	ULoadScreenSaveGame* SaveGame = RetrieveInGameSaveData();
	if (IsValid(SaveGame) == false)
		return;

	UGameplayStatics::LoadGameFromSlot(SaveGame->SlotName, SaveGame->SlotIndex);
}

void AAuraGameModeBase::RestartGameFromSaveDataWithWorldContextObject(UObject* WorldContextObject)
{
	// 저장 오브젝트 불러와 마지막 저장 확인
	ULoadScreenSaveGame* SaveGame = RetrieveInGameSaveData();
	if (IsValid(SaveGame) == false)
		return;

	UGameplayStatics::OpenLevel(WorldContextObject, FName(SaveGame->MapAssetName));
}

void AAuraGameModeBase::HandleInitializeCards(APlayerController* PC)
{
	if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(PC))
	{
		if (AAuraPlayerState* AuraPS = AuraPC->GetPlayerState<AAuraPlayerState>())
		{
			auto RandomUpgradeInfos = GetRandomUpgradeInfosForActivatedAbility_Three(AuraPS);
			AuraPS->SetUpgradeCardInfo(RandomUpgradeInfos);

			// 카드 내 선택 버튼 브로드캐스트
			AuraPC->OnCardSelectedDelegate.Broadcast();
		}
	}
}

void AAuraGameModeBase::HandleRandomUpgradeTagsGenerated(AAuraPlayerState* AuraPS,
                                                         TArray<FGameplayTag>& RandomUpgradeTags)
{
	if (!HasAuthority())
		return;

	if (RandomUpgradeTags.IsEmpty() || RandomUpgradeTags.Num() < 3)
		return;
	

	auto Tag0 = RandomUpgradeTags[0];
	auto Tag1 = RandomUpgradeTags[1];
	auto Tag2 = RandomUpgradeTags[2];

	auto* Info = UAuraAbilitySystemLibrary::GetAbilityUpgradeInfo(this);

	TArray<FAuraAbilityUpgradeInfo> UpgradeInfos;
	UpgradeInfos.Empty();
		
	UpgradeInfos.Add(Info->GetUpgradeInfoForUpgradeTag(Tag0));
	UpgradeInfos.Add(Info->GetUpgradeInfoForUpgradeTag(Tag1));
	UpgradeInfos.Add(Info->GetUpgradeInfoForUpgradeTag(Tag2));
		
	AuraPS->SetUpgradeCardInfo(UpgradeInfos);
}

void AAuraGameModeBase::HandlePlayerStateInitialized(AAuraPlayerState* InitializedPlayerState)
{

}

TArray<FAuraAbilityUpgradeInfo> AAuraGameModeBase::GetRandomUpgradeInfosForActivatedAbility_Three(
	AAuraPlayerState* AuraPS)
{
    TArray<FGameplayTag> ActivatedAbilityTags = AuraPS->GetAllActiveAbilityTags();
    TArray<FGameplayTag> InActivatedAbilityTags;
	InActivatedAbilityTags.Empty();
    
    TArray<FAuraAbilityUpgradeInfo> RandomUpgradeInfos;
    RandomUpgradeInfos.Empty();

    // 글로벌 업그레이드 할당
    ActivatedAbilityTags.AddUnique(FGameplayTag::RequestGameplayTag("Abilities.Fire"));
    ActivatedAbilityTags.AddUnique(FGameplayTag::RequestGameplayTag("Abilities.Arcane"));
    ActivatedAbilityTags.AddUnique(FGameplayTag::RequestGameplayTag("Abilities.Lightning"));

    // 어빌리티 습득 또는 레벨업 업그레이드 할당
    TArray<FGameplayTag> AllAbilitiesTags = FAuraGameplayTags::Get().GameplayAbilitiesTags;
    for (auto AbilityTag : AllAbilitiesTags)
    {
        InActivatedAbilityTags.AddUnique(AbilityTag);
    }

	while (RandomUpgradeInfos.Num() < 3)
	{
		// 주어진 어빌리티 태그에 대해 랜덤한 업그레이드 태그 뽑기
		if (UAbilityUpgradeInfo* Info = AbilityUpgradeInfo)
		{
			int CommonProbability = Info->UpgradeProbability[EUpgradeRarity::Common];
			int RareProbability = Info->UpgradeProbability[EUpgradeRarity::Rare];
			int UniqueProbability = Info->UpgradeProbability[EUpgradeRarity::Unique];
			int LegendaryProbability = Info->UpgradeProbability[EUpgradeRarity::Legendary];
			int SumProbability = CommonProbability + RareProbability + UniqueProbability + LegendaryProbability;

			int RandProbability = FMath::RandRange(0, SumProbability);

			if (RandProbability <= CommonProbability)
			{
				// 일반
				TArray<FAuraAbilityUpgradeInfo> AvailableUpgrades = Info->GetAvailableUpgradeInfoForTag(ActivatedAbilityTags, EUpgradeRarity::Common);
    		
				int RandInt = FMath::RandRange(0, AvailableUpgrades.Num() - 1);
				if (AvailableUpgrades.Num() > 0)
				{
    				RandomUpgradeInfos.AddUnique(AvailableUpgrades[RandInt]);
				}
				else
				{
					int RandInActiveAbility = FMath::RandRange(0, InActivatedAbilityTags.Num() - 1);
					if (InActivatedAbilityTags.Num() > 0)
						RandomUpgradeInfos.AddUnique(Info->GetUpgradeInfoForUpgradeTag(InActivatedAbilityTags[RandInActiveAbility]));
				}
			}
			else if (RandProbability <= CommonProbability + RareProbability)
			{
				// 레어
				TArray<FAuraAbilityUpgradeInfo> AvailableUpgrades = Info->GetAvailableUpgradeInfoForTag(ActivatedAbilityTags, EUpgradeRarity::Rare);
    		
				int RandInt = FMath::RandRange(0, AvailableUpgrades.Num() - 1);
				if (AvailableUpgrades.Num() > 0)
				{
					RandomUpgradeInfos.AddUnique(AvailableUpgrades[RandInt]);
				}
				else
				{
					int RandInActiveAbility = FMath::RandRange(0, InActivatedAbilityTags.Num() - 1);
					if (InActivatedAbilityTags.Num() > 0)
						RandomUpgradeInfos.AddUnique(Info->GetUpgradeInfoForUpgradeTag(InActivatedAbilityTags[RandInActiveAbility]));
				}
			}
			else if (RandProbability <= SumProbability - LegendaryProbability)
			{
				// 유니크 등급 카드
				TArray<FAuraAbilityUpgradeInfo> AvailableUpgrades = Info->GetAvailableUpgradeInfoForTag(ActivatedAbilityTags, EUpgradeRarity::Unique);
    		
				int RandInt = FMath::RandRange(0, AvailableUpgrades.Num() - 1);
				if (AvailableUpgrades.Num() > 0)
				{
					RandomUpgradeInfos.AddUnique(AvailableUpgrades[RandInt]);
				}
				else
				{
					int RandInActiveAbility = FMath::RandRange(0, InActivatedAbilityTags.Num() - 1);
					if (InActivatedAbilityTags.Num() > 0)
						RandomUpgradeInfos.AddUnique(Info->GetUpgradeInfoForUpgradeTag(InActivatedAbilityTags[RandInActiveAbility]));
				}
			}
			else if (RandProbability <= SumProbability)
			{
				// 전설 등급 카드
				TArray<FAuraAbilityUpgradeInfo> AvailableUpgrades = Info->GetAvailableUpgradeInfoForTag(ActivatedAbilityTags, EUpgradeRarity::Legendary);
    		
				int RandInt = FMath::RandRange(0, AvailableUpgrades.Num() - 1);
				if (AvailableUpgrades.Num() > 0)
				{
					RandomUpgradeInfos.AddUnique(AvailableUpgrades[RandInt]);
				}
				else
				{
					int RandInActiveAbility = FMath::RandRange(0, InActivatedAbilityTags.Num() - 1);
					if (InActivatedAbilityTags.Num() > 0)
						RandomUpgradeInfos.AddUnique(Info->GetUpgradeInfoForUpgradeTag(InActivatedAbilityTags[RandInActiveAbility]));
				}
			}
		}

		// 유효하지 않은 카드 제거
		for (auto It = RandomUpgradeInfos.CreateIterator(); It; ++It)
		{
			if (It->UpgradeEffectTag == FGameplayTag::EmptyTag)
			{
				It.RemoveCurrent();
			}
		}	

		// 최대 스택을 넘은 업그레이드가 있으면 제거
		for (auto It = RandomUpgradeInfos.CreateIterator(); It; ++It)
		{
			const FGameplayTag& UpgradeTag = It->UpgradeEffectTag;
			int32 StackCount = UAuraAbilitySystemLibrary::GetAbilityUpgradeStackCountByAuraPS(AuraPS, UpgradeTag);
			if (StackCount >= It->MaxStack)
			{
				It.RemoveCurrent();
			}
		}	
	}
    return RandomUpgradeInfos;
}

bool AAuraGameModeBase::GiveItemToCharacter(AAuraCharacter* Character, FName ItemID, int ItemCount)
{
	if (UInventoryComponent* Inventory = IPlayerInterface::Execute_GetInventoryComponent(Character))
	{
		if (UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>())
		{
			if (const FItemData* FoundRow = AuraGI->GetItemData(ItemID))
			{
				UAuraAbilitySystemLibrary::AddMessageToActor(FGameplayTag::RequestGameplayTag("Message.GetItem"), Character, FoundRow->DisplayName, FoundRow->Image);
				return Inventory->AddItem_Internal(*FoundRow, ItemCount);
			}
		}
	}
	return false;
}

void AAuraGameModeBase::SpawnDropItemActor(AAuraCharacter* OwnedCharacter, const FItemData& DropItemData, FVector ItemSpawnLocation)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 RandValue = FMath::RandRange(0, 6);
	int32 RandValue2 = FMath::RandRange(0, RandValue);
	
	float RandLocationX = FMath::RandRange(0, 50);
	float RandLocationY = FMath::RandRange(0, 50);
	
	ItemSpawnLocation.X += RandLocationX;
	ItemSpawnLocation.Y += RandLocationY;
	
	TArray<FRotator> ItemSpawnRotation = UAuraAbilitySystemLibrary::EvenlySpacedRotators(ItemSpawnLocation.ForwardVector, FVector::UpVector, 360.f, RandValue);
	FRotator RandRotation = ItemSpawnRotation[FMath::Clamp(RandValue2 - 1, 0, 6)];
	
	auto DropItemActor = GetWorld()->SpawnActor<AAuraDropItem>(
		DropItemClass,
		ItemSpawnLocation,
		RandRotation,
		SpawnParams);

	if (DropItemActor)
	{
		DropItemActor->InitializeItem(DropItemData);
		DropItemActor->SetItemCount(DropItemData.ItemCounts); // 아이템 갯수
		DropItemActor->SetOwner(OwnedCharacter);
	}
}

void AAuraGameModeBase::SpawnDropItemToActorLocation(AAuraCharacter* Character, FName ItemID)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	FVector ItemSpawnLocation = Character->GetActorLocation();

	int32 RandValue = FMath::RandRange(0, 6);
	int32 RandValue2 = FMath::RandRange(0, RandValue);
	
	float RandLocationX = FMath::RandRange(0, 50);
	float RandLocationY = FMath::RandRange(0, 50);
	
	ItemSpawnLocation.X += RandLocationX;
	ItemSpawnLocation.Y += RandLocationY;
	
	FItemData DropItemData = UAuraAbilitySystemLibrary::GetItemDataByItemName(this, ItemID);
	
	TArray<FRotator> ItemSpawnRotation = UAuraAbilitySystemLibrary::EvenlySpacedRotators(ItemSpawnLocation.ForwardVector, FVector::UpVector, 360.f, RandValue);
	FRotator RandRotation = ItemSpawnRotation[FMath::Clamp(RandValue2 - 1, 0, 6)];
	
	auto DropItemActor = GetWorld()->SpawnActor<AAuraDropItem>(
		DropItemClass,
		ItemSpawnLocation,
		RandRotation,
		SpawnParams);

	if (DropItemActor)
	{
		DropItemActor->InitializeItem(DropItemData);
		DropItemActor->SetItemCount(1); // 아이템 갯수
		DropItemActor->SetOwner(Character);
	}
}

void AAuraGameModeBase::DropItemOnMonsterDied(AAuraEnemy* DeadEnemy, AAuraCharacter* KilledBy)
{
	if (!DeadEnemy || !KilledBy)
		return;
	
	if (!HasAuthority())
		return;
	
	ECharacterClass EnemyCharacterClass = ICombatInterface::Execute_GetCharacterClass(DeadEnemy);
	
	// 아이템 드랍 테이블 가져오기
	if (auto AuraGI = GetGameInstance<UAuraGameInstance>())
	{
		if (auto ItemInfos = AuraGI->GetItemInfos())
		{
			auto DropList = ItemInfos->DropList;
			
			// 그룹 별 아이템 드랍 확률
			if (const FDropItemGroupArray* ItemGroupArray = ItemInfos->GetDropItemGroup(EnemyCharacterClass))
			{
				// 해당하는 몬스터 클래스의 아이템 그룹들 가져오기
				const TArray<FDropItemGroup>& ItemGroups = ItemGroupArray->Groups;
				const int32 DropCounts = ItemGroupArray->DropCounts;
				
				// 그룹 뽑기, 기본적으로 아이템 그룹은 '기타'
				EItemGroup SelectedItemGroup = EItemGroup::ETC;
			
				// 전체 아이템 그룹의 확률 합산 - 가중치 확률 분모 구하기
				float AllItemGroupProbability = 0.f;
				for (const FDropItemGroup& ItemGroup : ItemGroups)
				{
					AllItemGroupProbability += ItemGroup.GroupProbability;
				}
			
				// 동시 드랍 아이템 갯수만큼 스폰 반복
				for (int i = 0; i < DropCounts; i++)
				{
					float RandGroupValue = FMath::RandRange(0.f, AllItemGroupProbability);
					float SumGroupProbability = 0.f;
			
					// 아이템 그룹 선택
					for (const FDropItemGroup& ItemGroup : ItemGroups)
					{
						// usable->Equipment->Charm->ETC 순으로 계산
						SumGroupProbability += ItemGroup.GroupProbability;
				
						// 당첨
						if (RandGroupValue <= SumGroupProbability)
						{
							SelectedItemGroup = ItemGroup.ItemGroup;
							break;
						}
					}
			
					// 선택된 아이템 그룹의 확률 합산 - 가중치 확률 분모 구하기
					float AllItemsInSelectedGroupProbability = 0.f;
					for (const FDropItemGroup& ItemGroup : ItemGroups)
					{
						for (auto Item : ItemGroup.Items)
						{
							auto DropItemsGroup = ItemGroup.ItemGroup;
						
							// 이미 당첨된 아이템 그룹이 아니면 넘어감
							if (DropItemsGroup != SelectedItemGroup)
								continue;
						
							auto DropItemProbability = Item.DropProbability;
							
							AllItemsInSelectedGroupProbability += DropItemProbability;
						}
					}
			
					// 엣지 케이스
					if (FMath::IsNearlyZero(AllItemsInSelectedGroupProbability))
					{
						switch (SelectedItemGroup)
						{
						case EItemGroup::Charm :
							UE_LOG(LogTemp, Warning, TEXT("%s 몬스터의 드롭 아이템 그룹 'Charm'에 아이템이 존재하지 않습니다."), *DeadEnemy->GetName());
							continue;
						case EItemGroup::Equipment :
							UE_LOG(LogTemp, Warning, TEXT("%s 몬스터의 드롭 아이템 그룹 'Equipment'에 아이템이 존재하지 않습니다."), *DeadEnemy->GetName());
							continue;
						case EItemGroup::Usable :
							UE_LOG(LogTemp, Warning, TEXT("%s 몬스터의 드롭 아이템 그룹 'Usable'에 아이템이 존재하지 않습니다."), *DeadEnemy->GetName());
							continue;
						case EItemGroup::ETC :
							UE_LOG(LogTemp, Warning, TEXT("%s 몬스터의 드롭 아이템 그룹 'ETC'에 아이템이 존재하지 않습니다."), *DeadEnemy->GetName());
							continue;
						}
					}
				
					float RandItemValue = FMath::RandRange(0.f, AllItemsInSelectedGroupProbability);
					float SumItemProbability = 0.f;
					FItemData DropItemData;
			
					// 데이터 테이블의 행 핸들과 확률을 가짐
					for (const FDropItemGroup& ItemGroup : ItemGroups)
					{
						for (const FDropItemProbability& Item : ItemGroup.Items)
						{
							auto DropItemsHandle = Item.ItemHandle;
							auto DropItemsInfo = DropItemsHandle.GetRow<FItemData>("ItemInfo");
							auto DropItemsGroup = DropItemsInfo->ItemGroup;
				
							// 이미 당첨된 아이템 그룹이 아니면 넘어감
							if (DropItemsGroup != SelectedItemGroup)
								continue;
				
							auto DropItemProbability = Item.DropProbability;
				
							// 아이템 뽑기
							SumItemProbability += DropItemProbability;
							if (RandItemValue <= SumItemProbability)
							{
								DropItemData = *DropItemsInfo;
								break;
							}
						}
					}
					// 월드에 아이템 드랍
					FVector ItemSpawnLocation = DeadEnemy->GetActorLocation();
					SpawnDropItemActor(KilledBy, DropItemData, ItemSpawnLocation);
				}
			}
		}
	}
}

void AAuraGameModeBase::AddAbilityUpgradeToEnemy(TSubclassOf<UGameplayEffect> AbilityUpgradeClass, AActor* ApplyActor)
{
	if (AAuraEnemy* Enemy = Cast<AAuraEnemy>(ApplyActor))
	{
		Enemy->AddAbilityUpgrade(AbilityUpgradeClass);
	}
}

void AAuraGameModeBase::RemoveAbilityUpgradeFromEnemy(TSubclassOf<UGameplayEffect> AbilityUpgradeClass,
	AActor* ApplyActor)
{
	if (AAuraEnemy* Enemy = Cast<AAuraEnemy>(ApplyActor))
	{
		Enemy->RemoveAbilityUpgrade(AbilityUpgradeClass);
	}
}
