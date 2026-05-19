// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameModeBase.h"

#include "AbilitySystemBlueprintLibrary.h"
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
#include "Game/AuraGameStateBase.h"
#include "Interaction/SaveInterface.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Interaction/PlayerInterface.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"

AAuraGameModeBase::AAuraGameModeBase()
{
	bUseSeamlessTravel = true;
}

void AAuraGameModeBase::BeginPlay()
{
	Super::BeginPlay();

	OnAllActorsInvincible.AddDynamic(this, &AAuraGameModeBase::SetAllActorsInvincible);
	OnSetActorInvincible.AddDynamic(this, &AAuraGameModeBase::SetActorInvincible);
	
	// 리슨 서버를 플레이어 배열에 추가
	if (AAuraPlayerController* HostPC = Cast<AAuraPlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		if (AAuraGameStateBase* AuraGS = GetGameState<AAuraGameStateBase>())
		{
			AuraGS->AddPlayerToArray(HostPC);
		}
	}
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
		{
			if (AAuraGameStateBase* AuraGameState = GetGameState<AAuraGameStateBase>())
			{
				AuraGameState->AddPlayerToArray(AuraPC);
			}
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
		Actors.Sort([](const AActor& A, const AActor& B) 
		{
			const APlayerStart* StartA = Cast<APlayerStart>(&A);
			const APlayerStart* StartB = Cast<APlayerStart>(&B);
				
			// 태그를 비교
			if (StartA && StartB)
			{
				return StartA->PlayerStartTag.ToString() < StartB->PlayerStartTag.ToString();
			}
				return false;
		});
		
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

void AAuraGameModeBase::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(NewPlayer);
	if (!AuraPC)
		return;
	
	UAbilitySystemComponent* ASC = AuraPC->GetASC();
	if (!ASC)
		return;
	
	UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return;
}

void AAuraGameModeBase::RestartPlayerAtPlayerStart(AController* NewPlayer, AActor* StartSpot)
{
	Super::RestartPlayerAtPlayerStart(NewPlayer, StartSpot);
	
	AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(NewPlayer);
	if (!AuraPC)
		return;
	
	UAbilitySystemComponent* ASC = AuraPC->GetASC();
	if (!ASC)
		return;
	
	UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return;
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
	// UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(GetGameInstance())
	//
	// const FString InGameLoadSlotName = AuraGameInstance->LoadSlotName;
	// const int32 InGameLoadSlotIndex = AuraGameInstance->LoadSlotIndex;
	// AuraGameInstance->PlayerStartTag = SaveObject->PlayerStartTag;
	//
	// UGameplayStatics::SaveGameToSlot(SaveObject, InGameLoadSlotName, InGameLoadSlotIndex);
}

void AAuraGameModeBase::SaveAllCharacters(FName PlayerStartTag)
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(It->Get()))
		{
			AuraPC->SaveCharacterProgress(PlayerStartTag);
		}
	}
}

void AAuraGameModeBase::SaveWorldStateAndTravel(UWorld* World, const FString& DestinationMapAssetName)
{
	if (!HasAuthority())
		return;
	
	SaveWorldState(World, DestinationMapAssetName);
	ServerTravelToMap(DestinationMapAssetName);
}

void AAuraGameModeBase::SaveWorldState(UWorld* World, const FString& DestinationMapAssetName)
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

void AAuraGameModeBase::LoadWorldState(UWorld* World)
{
	if (!HasAuthority() || !World)
		return;

	// 세이브 데이터 가져오기
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
		
		if (Actor->Implements<UItemInterface>())
		{
			FGuid Guid = IItemInterface::Execute_GetGuid(Actor);
			bool bIsUsed = SaveGame->IsUsedActor(Guid);
			
		}

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

void AAuraGameModeBase::SaveOneTimeUseActor(FGuid Guid, bool bUsed)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
		return;
	
	if (UAuraGameInstance* AuraGI = PC->GetGameInstance<UAuraGameInstance>())
	{
		ULoadScreenSaveGame* SaveGame = AuraGI->GetSaveSlotData(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex);
		if (SaveGame)
		{
			SaveGame->OneTimeUseActors.Add(Guid, bUsed);
		}
	}
}

bool AAuraGameModeBase::IsOneTimeUseActorUsed(FGuid Guid)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
		return false;
	
	if (UAuraGameInstance* AuraGI = PC->GetGameInstance<UAuraGameInstance>())
	{
		ULoadScreenSaveGame* SaveGame = AuraGI->GetSaveSlotData(AuraGI->LoadSlotName, AuraGI->LoadSlotIndex);
		if (SaveGame)
		{
			if (bool* bUsed = SaveGame->OneTimeUseActors.Find(Guid))
			{
				return *bUsed;
			}
		}
	}
	
	return false;
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


void AAuraGameModeBase::GameAutoSave()
{
	// 월드 상태 저장
	UWorld* World = GetWorld();
	FString MapName = World->GetMapName();
	MapName.RemoveFromStart(World->StreamingLevelsPrefix);
	
	SaveWorldState(World, MapName);
	SaveAllCharacters();
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
	
	AAuraGameStateBase* AuraGameState = GetGameState<AAuraGameStateBase>();
	if (!AuraGameState)
		return;
    
	// 일반 몬스터 순회
	for (auto MonsterPtr : AuraGameState->GetEnemyCharactersArray()) 
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
	for (auto BossPtr : AuraGameState->GetBossCharactersArray())
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

void AAuraGameModeBase::SetActorInvincible(AActor* TargetActor, bool bInvincible)
{
	if (APawn* TargetPawn = Cast<APawn>(TargetActor))
	{
		if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(TargetPawn->GetController()))
		{
			// 플레이어 캐릭터
			AuraPC->Server_CharacterInvincible(bInvincible);
		}
		else if (AAuraAIController* AuraAI = Cast<AAuraAIController>(TargetPawn->GetController()))
		{
			// 몬스터 캐릭터
			AuraAI->Server_CharacterInvincible(bInvincible);
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
	
	UAbilitySystemComponent* ASC = DeadPC->GetASC();
	if (!ASC)
		return;
	
	// 부활 후 절반 체력 및 절반 마나 회복
	UAuraGameInstance* AuraGI = GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return;
	
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
	
	// 부활 이펙트 적용 - 체력/마나 절반 회복
	TSubclassOf<UGameplayEffect> ReviveEffectClass = AuraGI->ReviveEffect;
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ReviveEffectClass, 1.f, Context);
			
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	
	// 3초 무적
	FGameplayEffectContextHandle InvincibleContext = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle InvincibleSpecHandle = ASC->MakeOutgoingSpec(Invincible3Sec, 1.f, InvincibleContext);
	ASC->ApplyGameplayEffectSpecToSelf(*InvincibleSpecHandle.Data.Get());
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

FAuraAbilityUpgradeInfo AAuraGameModeBase::GetUpgradeRecursive(EUpgradeRarity Rarity,
	TMap<EUpgradeRarity, TArray<FAuraAbilityUpgradeInfo>>& Buckets)
{
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return FAuraAbilityUpgradeInfo();
	
	UAbilityUpgradeInfo* Info = AuraGI->GetAbilityUpgradeInfo();
	if (!Info)
		return FAuraAbilityUpgradeInfo();
	
	// 등급에 해당하는 바구니 가져오기
	TArray<FAuraAbilityUpgradeInfo>* Bucket = Buckets.Find(Rarity);

	// 현재 등급에 뽑을 게 있다면 반환
	if (Bucket && Bucket->Num() > 0)
	{
		int32 RandInt = FMath::RandRange(0, Bucket->Num() - 1);
		FAuraAbilityUpgradeInfo SelectedCard = (*Bucket)[RandInt];
		Bucket->RemoveAt(RandInt);
		
		if (SelectedCard.UpgradeEffectTag.IsValid())
		{
			return SelectedCard;
		}
	}
	
	// 지금 등급 바구니에 아이템이 없지만, 아래 등급 바구니가 남아 있는 경우 재귀 호출
	// enum->uint8, -1을 더하고 다시 enum으로 변환
	if (Rarity > EUpgradeRarity::Common)
	{
		EUpgradeRarity LowerRarity = static_cast<EUpgradeRarity>(static_cast<uint8>(Rarity) - 1);
		return GetUpgradeRecursive(LowerRarity, Buckets);
	}
	
	// 뽑은 등급이 제일 마지막인데 비어 있으면 기본 데미지 업그레이드 반환
	if (Rarity == EUpgradeRarity::Common)
	{
		// 일반 바구니가 비었음 -> 데미지 업그레이드 뽑아서 반환
		TArray<FGameplayTag> DamageUpgradeTags;
		DamageUpgradeTags.Add(FGameplayTag::RequestGameplayTag("Abilities.Fire"));
		DamageUpgradeTags.Add(FGameplayTag::RequestGameplayTag("Abilities.Arcane"));
		DamageUpgradeTags.Add(FGameplayTag::RequestGameplayTag("Abilities.Lightning"));
		
		int RandInt = FMath::RandRange(0, DamageUpgradeTags.Num() - 1);
		FGameplayTag SelectedDamageTag = DamageUpgradeTags[RandInt];
		
		return Info->GetUpgradesForAbility(SelectedDamageTag).UpgradeInfos[0];
	}
	
	return FAuraAbilityUpgradeInfo();
}

TArray<FAuraAbilityUpgradeInfo> AAuraGameModeBase::GetRandomUpgradeInfosForActivatedAbility_Three(
	AAuraPlayerState* AuraPS)
{
	TArray<FAuraAbilityUpgradeInfo> UpgradeCards;
	UpgradeCards.Empty();
	
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return UpgradeCards;

	// 주어진 어빌리티 태그에 대해 랜덤한 업그레이드 태그 뽑기
	UAbilityUpgradeInfo* Info = AuraGI->GetAbilityUpgradeInfo();
	if (!Info)
		return UpgradeCards;
	
	TArray<FGameplayTag> DamageUpgradeTags =
	{
		FGameplayTag::RequestGameplayTag("Abilities.Fire"),
		FGameplayTag::RequestGameplayTag("Abilities.Arcane"),
		FGameplayTag::RequestGameplayTag("Abilities.Lightning")
	};
	
	// 등급 바구니 생성
	TMap<EUpgradeRarity, TArray<FAuraAbilityUpgradeInfo>> RarityBucket;
	TArray<FGameplayTag> ActiveTags = AuraPS->GetAllActiveAbilityTags();
	TArray<EUpgradeRarity> Rarities = {EUpgradeRarity::Common, EUpgradeRarity::Rare, EUpgradeRarity::Unique, EUpgradeRarity::Legendary};
	
	// 활성화 된 어빌리티의 업그레이드를 모아옴
	for (const auto Rarity : Rarities)
	{
		TArray<FAuraAbilityUpgradeInfo> AvailableUpgrades = Info->GetAvailableUpgradeInfoForTag(ActiveTags, Rarity);
		for (const auto& Upgrade : AvailableUpgrades)
		{
			// 업그레이드 배열을 찾아와 업그레이드 정보를 집어넣음
			RarityBucket.FindOrAdd(Rarity).Add(Upgrade);
		}
	}
	
	// 최대 스택을 넘은 업그레이드, 빈 태그 업그레이드가 있으면 바구니에서 제거
	for (auto It = RarityBucket.CreateIterator(); It; ++It)
	{
		auto& Bucket = It->Value;
		
		for (int i = Bucket.Num() - 1; i > 0; --i)
		{
			// 가져온 바구니에서 빈 태그 업그레이드 제거
			const FGameplayTag& UpgradeTag = Bucket[i].UpgradeEffectTag;
			if (UpgradeTag == FGameplayTag::EmptyTag || UpgradeTag == FAuraGameplayTags::Get().Abilities_None)
			{
				Bucket.RemoveAt(i);
				continue;
			}
			
			// 가져온 바구니의 뒤에서부터 스택을 초과한 업그레이드를 제거
			int32 StackCount = UAuraAbilitySystemLibrary::GetAbilityUpgradeStackCountByAuraPS(AuraPS, UpgradeTag);
			if (StackCount >= Bucket[i].MaxStack)
			{
				// 바구니에서 업그레이드 빼기
				Bucket.RemoveAt(i);
			}
		}
	}	

	// 3장 뽑기
	while (UpgradeCards.Num() < 3)
	{
		// 주사위 돌리기
		int CommonProbability = Info->UpgradeProbability[EUpgradeRarity::Common];
		int RareProbability = Info->UpgradeProbability[EUpgradeRarity::Rare];
		int UniqueProbability = Info->UpgradeProbability[EUpgradeRarity::Unique];
		int LegendaryProbability = Info->UpgradeProbability[EUpgradeRarity::Legendary];
		int SumProbability = CommonProbability + RareProbability + UniqueProbability + LegendaryProbability;

		int RandProbability = FMath::RandRange(0, SumProbability);
		
		EUpgradeRarity SelectedRarity = EUpgradeRarity::Common;
		if (RandProbability <= CommonProbability)
		{
			SelectedRarity = EUpgradeRarity::Common;
		}
		else if (RandProbability <= CommonProbability + RareProbability)
		{
			SelectedRarity = EUpgradeRarity::Rare;
		}
		else if (RandProbability <= SumProbability - LegendaryProbability)
		{
			SelectedRarity = EUpgradeRarity::Unique;
		}
		else if (RandProbability <= SumProbability)
		{
			SelectedRarity = EUpgradeRarity::Legendary;
		}
		
		auto UpgradeCard = GetUpgradeRecursive(SelectedRarity, RarityBucket);
		UpgradeCards.AddUnique(UpgradeCard);
	}
	return UpgradeCards;
}

void AAuraGameModeBase::SendXPToAllPlayers(AActor* KilledActor, AActor* KillerActor)
{
	if (!KilledActor->Implements<UCombatInterface>())
		return;
	
	// 사망한 캐릭터가 플레이어 캐릭터라면 XP를 제공하지 않음
	if (Cast<AAuraCharacter>(KilledActor))
		return;

	const int32 TargetLevel = ICombatInterface::Execute_GetCharacterLevel(KilledActor);
	const ECharacterClass TargetClass = ICombatInterface::Execute_GetCharacterClass(KilledActor);
	const int32 TotalXP = UAuraAbilitySystemLibrary::GetXPRewardForClassAndLevel(KilledActor, TargetClass, TargetLevel);

	// 모든 컨트롤러 순회
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
			continue;

		APawn* Pawn = PC->GetPawn();
		if (!Pawn)
			continue;

		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
		if (!ASC)
			continue;
		
		// 분배 방식에 따라 XP 조정
		int32 PlayerCount = GetNumPlayers();
		int32 KillerXP = TotalXP / FMath::Max(XPDistributionRatio, 1);
		int32 SharedXP = TotalXP / FMath::Max(PlayerCount, 1);
		FGameplayEventData Payload;
		Payload.EventTag = FAuraGameplayTags::Get().Attributes_Meta_IncomingXP;
		if (KillerActor && Pawn == KillerActor)
		{
			// 직접 몬스터를 처치한 플레이어가 더 많은 비율을 가져감
			Payload.EventMagnitude = KillerXP;
		}
		else
		{
			// 직접 처치하지 않았다면 절반만 가져감
			Payload.EventMagnitude = SharedXP;
		}

		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
			Pawn,
			FAuraGameplayTags::Get().Attributes_Meta_IncomingXP,
			Payload
		);
	}
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

	int32 RandValue = FMath::RandRange(3, 8);
	const float DropRadius = FMath::RandRange(150.f, 300.f); 
	
	FVector Offset = FVector(DropRadius, 0.f, 0.f).RotateAngleAxis(360.f / RandValue, FVector::UpVector);
	FVector FinalSpawnLocation = ItemSpawnLocation + Offset;
	
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return;
	
	auto DropItemActor = GetWorld()->SpawnActor<AAuraDropItem>(
		AuraGI->DropItemClass,
		FinalSpawnLocation,
		FRotator(),
		SpawnParams);

	if (DropItemActor)
	{
		DropItemActor->InitializeItem(DropItemData);
		DropItemActor->SetItemCount(DropItemData.ItemCounts);
		DropItemActor->SetOwner(OwnedCharacter);
	}
}

void AAuraGameModeBase::SpawnDropItemToActorLocation(AActor* Actor, FItemData ItemData)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	FVector ItemSpawnLocation = Actor->GetActorLocation();
	
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return;
	
	auto DropItemActor = GetWorld()->SpawnActor<AAuraDropItem>(
		AuraGI->DropItemClass,
		ItemSpawnLocation,
		FRotator(),
		SpawnParams);

	if (DropItemActor)
	{
		DropItemActor->InitializeItem(ItemData);
		DropItemActor->SetItemCount(1);
		DropItemActor->SetOwner(Actor);
	}
}

void AAuraGameModeBase::SpawnDropItemToLocation(FVector Location, FName ItemID)
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	FItemData DropItemData = UAuraAbilitySystemLibrary::GetItemDataByItemName(this, ItemID);
	
	// 아이템에 Guid 설정
	DropItemData.UniqueID = FGuid::NewGuid();
		
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return;
	
	auto DropItemActor = GetWorld()->SpawnActor<AAuraDropItem>(
		AuraGI->DropItemClass,
		Location,
		FRotator(),
		SpawnParams);

	if (DropItemActor)
	{
		DropItemActor->InitializeItem(DropItemData);
		DropItemActor->SetItemCount(1);
	}
}

void AAuraGameModeBase::DropItemOnMonsterDied(AAuraEnemy* DeadEnemy, AAuraCharacter* KilledBy)
{
	if (!DeadEnemy || !KilledBy)
		return;
	
	if (!HasAuthority())
		return;
	
	ECharacterClass EnemyCharacterClass = ICombatInterface::Execute_GetCharacterClass(DeadEnemy);
	auto AuraGI = GetGameInstance<UAuraGameInstance>();
	if (!AuraGI)
		return;
	
	// 아이템 드랍 테이블 가져오기
	auto ItemInfos = AuraGI->GetItemInfos();
	if (!ItemInfos)
		return;
	
	// 그룹 별 아이템 드랍 확률
	const FDropItemGroupArray* ItemGroupArray = ItemInfos->GetDropItemGroup(EnemyCharacterClass);
	if (!ItemGroupArray)
		return;
	
	// 해당하는 몬스터 클래스의 아이템 그룹들 가져오기
	const TArray<FDropItemGroup>& ItemGroups = ItemGroupArray->Groups;
	const int32 DropCounts = ItemGroupArray->DropCounts;
			
	// 전체 아이템 그룹의 확률 합산 - 가중치 확률 분모 구하기
	float GroupProbability = 0.f;
	for (const FDropItemGroup& ItemGroup : ItemGroups)
	{
		GroupProbability += ItemGroup.GroupProbability;
	}
			
	// 동시 드랍 아이템 갯수만큼 스폰 반복
	for (int i = 0; i < DropCounts; i++)
	{
		// 그룹 뽑기, 기본적으로 아이템 그룹은 '기타'
		EItemGroup SelectedGroup = EItemGroup::ETC;
		
		float RandGroupValue = FMath::RandRange(0.f, GroupProbability);
		float SumGroupProbability = 0.f;
			
		// 아이템 그룹 선택
		for (const FDropItemGroup& ItemGroup : ItemGroups)
		{
			SumGroupProbability += ItemGroup.GroupProbability;
				
			// 당첨
			if (RandGroupValue <= SumGroupProbability)
			{
				SelectedGroup = ItemGroup.ItemGroup;
				break;
			}
		}
					
		// None 그룹에 당첨될 경우 컨티뉴
		if (SelectedGroup == EItemGroup::None)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s : 꽝 당첨"), *DeadEnemy->GetName());
			continue;
		}
			
		// 선택된 아이템 그룹에 해당하는 아이템 모으기 및 확률 합산(가중치 확률 분모 구하기)
		TArray<const FDropItemProbability*> SelectedItems;
		float SelectedGroupItemsProbSum = 0.f;

		for (const auto& Group : ItemGroups)
		{
			if (Group.ItemGroup != SelectedGroup) continue;
			for (const auto& Item : Group.Items)
			{
				SelectedItems.Add(&Item);
				SelectedGroupItemsProbSum += Item.DropProbability;
			}
		}
			
		// 엣지 케이스
		if (FMath::IsNearlyZero(SelectedGroupItemsProbSum))
		{
			switch (SelectedGroup)
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
			case EItemGroup::SpellUpgrade :
				UE_LOG(LogTemp, Warning, TEXT("%s 몬스터의 드롭 아이템 그룹 'SpellUpgrade'에 아이템이 존재하지 않습니다."), *DeadEnemy->GetName());
				continue;
			case EItemGroup::None :
				continue;
			}
		}
				
		float RandItemValue = FMath::RandRange(0.f, SelectedGroupItemsProbSum);
		float SumItemProbability = 0.f;
		FItemData DropItemData;
		bool bFound = false;
		
		// 아이템 개별마다 지정된 드랍 갯수
		int32 ItemDropCounts = 1;
			
		// 아이템 후보 내에서 확률 계산
		for (const auto* Item : SelectedItems)
		{
			// 바구니 내의 아이템들의 종합 확률
			SumItemProbability += Item->DropProbability;
			ItemDropCounts = Item->DropCount;
			
			// 목록 중 최상단부터 계산하기
			if (RandItemValue <= SumItemProbability)
			{
				// 선택된 그룹이 스펠 업그레이드라면 업그레이드 카드를 생성하도록 요청하고 종료
				if (SelectedGroup == EItemGroup::SpellUpgrade)
				{
					for (int j = 0; j < ItemDropCounts; j++)
					{
						for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
						{
							if (APlayerController* PC = It->Get())
							{
								if (auto AuraPC = Cast<AAuraPlayerController>(PC))
								{
									AuraPC->Server_CreateCardSelection(AuraPC->GetPawn());
								}
							}
						}
					}
					return;
				}
				
				if (FItemData* RowData = Item->ItemHandle.GetRow<FItemData>("ItemInfo"))
				{
					DropItemData = *RowData;
					bFound = true;
				}
				break;
			}
		}
		
		// 행을 찾았을 때만 월드에 드랍
		if (bFound)
		{
			// 아이템에 Guid 설정
			DropItemData.UniqueID = FGuid::NewGuid();
			
			// 아이템 개별마다 지정된 드랍 갯수만큼 반복 드랍
			for (int j = 0; j < ItemDropCounts; j++)
			{
				// 월드에 아이템 드랍
				SpawnDropItemActor(KilledBy, DropItemData, DeadEnemy->GetActorLocation());
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
