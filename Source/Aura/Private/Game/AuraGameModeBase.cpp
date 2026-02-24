// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameModeBase.h"

#include "AuraGameplayTags.h"
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
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"

AAuraGameModeBase::AAuraGameModeBase()
{
	bUseSeamlessTravel = true;
}

void AAuraGameModeBase::BeginPlay()
{
	Super::BeginPlay();

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
		if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(NewPlayer))
			Players.AddUnique(AuraPC);
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

	return AuraGameInstance->GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
}

ULoadScreenSaveGame* AAuraGameModeBase::RetrieveInGameSaveData(APlayerController* PC)
{
	if (!PC)
		return nullptr;
	
	UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(PC->GetGameInstance());

	const FString InGameLoadSlotName = AuraGameInstance->LoadSlotName;
	const int32 InGameLoadSlotIndex = AuraGameInstance->LoadSlotIndex;

	return AuraGameInstance->GetSaveSlotData(InGameLoadSlotName, InGameLoadSlotIndex);
}

void AAuraGameModeBase::SaveInGameProgressData(ULoadScreenSaveGame* SaveObject)
{
	// UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance());
	//
	// const FString InGameLoadSlotName = AuraGameInstance->LoadSlotName;
	// const int32 InGameLoadSlotIndex = AuraGameInstance->LoadSlotIndex;
	// AuraGameInstance->PlayerStartTag = SaveObject->PlayerStartTag;
	//
	// UGameplayStatics::SaveGameToSlot(SaveObject, InGameLoadSlotName, InGameLoadSlotIndex);
}

void AAuraGameModeBase::Server_SaveWorldStateAndTravel_Implementation(UWorld* World,
	const FString& DestinationMapAssetName)
{
	if (!HasAuthority())
		return;
	
	Server_SaveWorldState(World, DestinationMapAssetName);
	ServerTravelToMap(DestinationMapAssetName);
}

void AAuraGameModeBase::Client_SaveCharacterProgress_Implementation()
{
	auto* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	AActor* Actor = Cast<AActor>(PlayerController->GetPawn());
	if (Actor->Implements<UPlayerInterface>())
		IPlayerInterface::Execute_SaveProgress(Actor, "");
}

void AAuraGameModeBase::Server_SaveWorldState_Implementation(UWorld* World, const FString& DestinationMapAssetName)
{
	if (!HasAuthority())
		return;
	
	// 접두사를 제외한 실제 이름만 가져오기
	FString WorldName = World->GetMapName();
	WorldName.RemoveFromStart(World->StreamingLevelsPrefix);

	UAuraGameInstance* AuraGI = Cast<UAuraGameInstance>(GetGameInstance());
	if (!AuraGI)
		return;
	
	if (ULoadScreenSaveGame* SaveGame = AuraGI->GetSaveSlotData(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex))
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
		for (FActorIterator It(World); It; ++It)
		{
			AActor* Actor = *It;

			// 사망 상태가 아닐 때 또는 저장 인터페이스를 상속받지 않을 때 패스
			if (IsValid(Actor) == false || Actor->Implements<USaveInterface>() == false)
				continue;
			
			if (Actor->Implements<UPlayerInterface>())
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

void AAuraGameModeBase::Server_LoadWorldState_Implementation(UWorld* World)
{
	if (!HasAuthority() || !World)
		return;

	// 1. 세이브 데이터 가져오기
	UAuraGameInstance* AuraGI = Cast<UAuraGameInstance>(GetGameInstance());
	if (!AuraGI)
		return;

	if (!UGameplayStatics::DoesSaveGameExist(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex))
		return;

	ULoadScreenSaveGame* SaveGame = Cast<ULoadScreenSaveGame>(UGameplayStatics::LoadGameFromSlot(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex));
	if (!SaveGame)
		return;
	
	FString WorldName = World->GetMapName();
	WorldName.RemoveFromStart(World->StreamingLevelsPrefix);
	
	// 저장된 맵 가져오기
	const FSavedMap& CurrentSavedMap = SaveGame->GetSavedMapWithMapName(WorldName);
    
	// 현재 맵의 저장된 액터 데이터 가져옴
	TMap<FName, const FSavedActor*> SavedActorMap;
	for (const FSavedActor& SavedActorData : CurrentSavedMap.SavedActors)
	{
		SavedActorMap.Add(SavedActorData.ActorName, &SavedActorData);
	}

	// 가져온 액터 순회
	for (FActorIterator It(World); It; ++It)
	{
		AActor* Actor = *It;

		if (!IsValid(Actor) || !Actor->Implements<USaveInterface>()) 
			continue;

		// Map에서 해당 액터의 데이터가 있는지 확인
		if (const FSavedActor* FoundData = SavedActorMap.FindRef(Actor->GetFName()))
		{
			// 트랜스폼 불러오기
			if (ISaveInterface::Execute_ShouldLoadTransform(Actor))
				Actor->SetActorTransform(FoundData->Transform);
			
			// 액터 불러오기
			ISaveInterface::Execute_LoadActor(Actor); 
		}
	}
}

void AAuraGameModeBase::Multicast_SaveCharacterProgress_Implementation()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	APawn* Pawn = PC->GetPawn();
	if (Pawn && Pawn->Implements<UPlayerInterface>())
	{
		IPlayerInterface::Execute_SaveProgress(Pawn, "");
	}
}

void AAuraGameModeBase::TravelToMap(UMVVM_LoadSlot* Slot)
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(Slot, Maps.FindChecked(Slot->GetMapName()), true, TEXT("?listen"));
}

void AAuraGameModeBase::TravelToMap(FString MapName)
{
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Maps.FindChecked(MapName), false, TEXT("?listen"));
}

void AAuraGameModeBase::ServerTravelToMap(FString MapName)
{
	if (auto Map = Maps.Find(MapName))
	{
		if (Map)
		{
			FString Path = Map->ToSoftObjectPath().GetLongPackageName();
			FString TravelURL = FString::Printf(TEXT("%s?listen"), *Path);
			GetWorld()->ServerTravel(TravelURL);
		}
	}
}


void AAuraGameModeBase::Server_GameAutoSave_Implementation()
{
	// 월드 상태 저장
	UWorld* World = GetWorld();
	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	
	Server_SaveWorldState(World, MapName);
	Multicast_SaveCharacterProgress();
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
	Server_GameAutoSave();
}

void AAuraGameModeBase::AddMonsterToArray(AAuraEnemy* Enemy)
{
	if (AAuraBossMonster* Boss = Cast<AAuraBossMonster>(Enemy))
	{
		BossCharacters.Add(Boss);
		OnBossMonsterCountChanged.Broadcast(BossCharacters.Num());

		// 보스 사망 델리게이트 구독 -> 게임 강제 저장
		Boss->OnDeath.AddDynamic(this, &AAuraGameModeBase::OnBossMonsterDead);

		if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
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

void AAuraGameModeBase::PlayerRespawn(AAuraPlayerController* DeadPC)
{
	if (!DeadPC)
		return;

	// 아직 안사라졌으면 확실히 제거
	if (APawn* DeadPawn = DeadPC->GetPawn())
	{
		DeadPC->UnPossess();
		DeadPawn->Destroy();
	}
	
	// 저장된 플레이어 스타트에서 재시작
	if (AActor* SavedPlayerStart = ChoosePlayerStart(DeadPC))
	{
		RestartPlayerAtPlayerStart(DeadPC, SavedPlayerStart);
	}
	else
	{
		// 저장된 플레이어 스타트가 없으면 그냥 부활
		RestartPlayer(DeadPC);
	}
	
	// 사망 태그 부여 이펙트 제거
	if (UAbilitySystemComponent* ASC = DeadPC->GetASC())
	{
		ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("State.Dead")));
		
		// 부활 후 절반 체력 및 절반 마나 회복
		if (UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>())
		{
			TSubclassOf<UGameplayEffect> ReviveEffectClass = AuraGI->ReviveEffect;
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ReviveEffectClass, 1.f, Context);
			
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void AAuraGameModeBase::HandleInitializeCards(APlayerController* PC)
{
	if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(PC))
	{
		if (AAuraPlayerState* AuraPS = AuraPC->GetPlayerState<AAuraPlayerState>())
		{
			auto RandomUpgradeInfos = GetRandomUpgradeInfosForActivatedAbility_Three(AuraPS);
			AuraPS->SetUpgradeCardInfo(RandomUpgradeInfos);
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
		UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
		if (AuraGI == nullptr)
			return TArray<FAuraAbilityUpgradeInfo>();

		// 주어진 어빌리티 태그에 대해 랜덤한 업그레이드 태그 뽑기
		if (UAbilityUpgradeInfo* Info = AuraGI->GetAbilityUpgradeInfo())
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

bool AAuraGameModeBase::GiveItemToCharacter(AAuraCharacter* Character, const FItemData& ItemData, int ItemCount)
{
	if (UInventoryComponent* Inventory = IPlayerInterface::Execute_GetInventoryComponent(Character))
	{
		UAuraAbilitySystemLibrary::AddMessageToActor(Character, FGameplayTag::RequestGameplayTag("Message.GetItem"), ItemData.DisplayName, ItemData.Image);
		return Inventory->AddItem_Internal(ItemData, ItemCount);
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
	
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return;
	
	auto DropItemActor = GetWorld()->SpawnActor<AAuraDropItem>(
		AuraGI->DropItemClass,
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

void AAuraGameModeBase::SpawnDropItemToActorLocation(AActor* Actor, FName ItemID)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	FVector ItemSpawnLocation = Actor->GetActorLocation();

	int32 RandValue = FMath::RandRange(0, 6);
	int32 RandValue2 = FMath::RandRange(0, RandValue);
	
	float RandLocationX = FMath::RandRange(0, 50);
	float RandLocationY = FMath::RandRange(0, 50);
	
	ItemSpawnLocation.X += RandLocationX;
	ItemSpawnLocation.Y += RandLocationY;
	
	FItemData DropItemData = UAuraAbilitySystemLibrary::GetItemDataByItemName(this, ItemID);
	
	// 아이템에 Guid 설정
	DropItemData.UniqueID = FGuid::NewGuid();
	TArray<FRotator> ItemSpawnRotation = UAuraAbilitySystemLibrary::EvenlySpacedRotators(ItemSpawnLocation.ForwardVector, FVector::UpVector, 360.f, RandValue);
	FRotator RandRotation = ItemSpawnRotation[FMath::Clamp(RandValue2 - 1, 0, 6)];
	
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return;
	
	auto DropItemActor = GetWorld()->SpawnActor<AAuraDropItem>(
		AuraGI->DropItemClass,
		ItemSpawnLocation,
		RandRotation,
		SpawnParams);

	if (DropItemActor)
	{
		DropItemActor->InitializeItem(DropItemData);
		DropItemActor->SetItemCount(1); // 아이템 갯수
		DropItemActor->SetOwner(Actor);
	}
}

void AAuraGameModeBase::SpawnDropItemToLocation(FVector Location, FName ItemID)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 RandValue = FMath::RandRange(0, 6);
	int32 RandValue2 = FMath::RandRange(0, RandValue);
	
	float RandLocationX = FMath::RandRange(0, 50);
	float RandLocationY = FMath::RandRange(0, 50);
	
	Location.X += RandLocationX;
	Location.Y += RandLocationY;
	
	FItemData DropItemData = UAuraAbilitySystemLibrary::GetItemDataByItemName(this, ItemID);
	
	// 아이템에 Guid 설정
	DropItemData.UniqueID = FGuid::NewGuid();
	TArray<FRotator> ItemSpawnRotation = UAuraAbilitySystemLibrary::EvenlySpacedRotators(Location.ForwardVector, FVector::UpVector, 360.f, RandValue);
	FRotator RandRotation = ItemSpawnRotation[FMath::Clamp(RandValue2 - 1, 0, 6)];
		
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return;
	
	auto DropItemActor = GetWorld()->SpawnActor<AAuraDropItem>(
		AuraGI->DropItemClass,
		Location,
		RandRotation,
		SpawnParams);

	if (DropItemActor)
	{
		DropItemActor->InitializeItem(DropItemData);
		DropItemActor->SetItemCount(1); // 아이템 갯수
		// DropItemActor->SetOwner();
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
	
					// 아이템에 Guid 설정
					DropItemData.UniqueID = FGuid::NewGuid();
					
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
