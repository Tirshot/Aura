// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_LoadScreen.h"
#include "UI/ViewModel/MVVM_LoadSlot.h"
#include "Game/AuraGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "Game/AuraGameInstance.h"

void UMVVM_LoadScreen::InitializeLoadSlots()
{
	LoadSlot_0 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
	LoadSlot_0->SetLoadSlotName("LoadSlot_0");
	LoadSlot_0->SlotIndex = 0;

	LoadSlot_1 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
	LoadSlot_1->SetLoadSlotName("LoadSlot_1");
	LoadSlot_1->SlotIndex = 1;

	LoadSlot_2 = NewObject<UMVVM_LoadSlot>(this, LoadSlotViewModelClass);
	LoadSlot_2->SetLoadSlotName("LoadSlot_2");
	LoadSlot_2->SlotIndex = 2;

	// PIE에서는 파일 이름 구분
	if (UE::GetPlayInEditorID() != -1)
	{
		FString LoadSlotName_0 = FString::Printf(TEXT("Player%d_LoadSlot_0"), UE::GetPlayInEditorID());
		FString LoadSlotName_1 = FString::Printf(TEXT("Player%d_LoadSlot_1"), UE::GetPlayInEditorID());
		FString LoadSlotName_2 = FString::Printf(TEXT("Player%d_LoadSlot_2"), UE::GetPlayInEditorID());
		LoadSlot_0->SetLoadSlotName(LoadSlotName_0);
		LoadSlot_1->SetLoadSlotName(LoadSlotName_1);
		LoadSlot_2->SetLoadSlotName(LoadSlotName_2);
	}
	
	LoadSlots.Add(0, LoadSlot_0);
	LoadSlots.Add(1, LoadSlot_1);
	LoadSlots.Add(2, LoadSlot_2);

	SetNumLoadSlots(LoadSlots.Num());
}

UMVVM_LoadSlot* UMVVM_LoadScreen::GetLoadSlotViewModelByIndex(int32 Index)
{
	return LoadSlots.FindChecked(Index);
}

void UMVVM_LoadScreen::NewSlotButtonPressed(int32 Slot, const FString& EnteredName)
{
	if (UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		// 새로운 슬롯 생성
		auto& LoadSlot = LoadSlots[Slot];
		
		LoadSlot->SetMapName(AuraGI->DefaultMapName);
		LoadSlot->SetLevel(1);
		LoadSlot->SetPlayerName(EnteredName);
		LoadSlot->SlotStatus = Taken;
		LoadSlot->PlayerStartTag = AuraGI->DefaultPlayerStartTag;
		LoadSlot->MapAssetName = AuraGI->DefaultMap.ToSoftObjectPath().GetAssetName();

		// 이미 게임이 존재하면 데이터 삭제
		if (UGameplayStatics::DoesSaveGameExist(LoadSlot->GetLoadSlotName(), Slot))
		{
			UGameplayStatics::DeleteGameInSlot(LoadSlot->GetLoadSlotName(), Slot);
		}

		// 저장 오브젝트 생성
		USaveGame* SaveGameObject = UGameplayStatics::CreateSaveGameObject(AuraGI->LoadScreenSaveGameClass);

		// 데이터를 집어넣기
		ULoadScreenSaveGame* LoadScreenSaveGame = Cast<ULoadScreenSaveGame>(SaveGameObject);
		LoadScreenSaveGame->PlayerName = LoadSlot->GetPlayerName();
		LoadScreenSaveGame->MapName = LoadSlot->GetMapName();
		LoadScreenSaveGame->SaveSlotStatus = Taken;
		LoadScreenSaveGame->PlayerStartTag = LoadSlot->PlayerStartTag;
		LoadScreenSaveGame->MapAssetName = LoadSlot->MapAssetName;
		
		// 최종 저장
		UGameplayStatics::SaveGameToSlot(LoadScreenSaveGame, LoadSlot->GetLoadSlotName(), Slot);
		
		LoadSlots[Slot]->InitializeSlot();

		// 게임 인스턴스로 값 넘기기
		AuraGI->LoadSlotName = LoadSlots[Slot]->GetLoadSlotName();
		AuraGI->LoadSlotIndex = LoadSlots[Slot]->SlotIndex;
	}
}

void UMVVM_LoadScreen::NewGameButtonPressed(int32 Slot)
{
	// 위젯 스위처 전환
	LoadSlots[Slot]->SetWidgetSwitcherIndex.Broadcast(1);
}

void UMVVM_LoadScreen::SelectSlotButtonPressed(int32 Slot)
{
	// 슬롯 선택
	SlotSelected.Broadcast();
	
	SelectedSlotIndex = Slot;
	for (const TPair<int32, UMVVM_LoadSlot*> LoadSlot : LoadSlots)
	{
		// 다른 슬롯 선택 비활성화
		if (LoadSlot.Key == Slot)
		{
			LoadSlot.Value->EnableSelectSlotButton.Broadcast(false);
		}
		else
		{
			LoadSlot.Value->EnableSelectSlotButton.Broadcast(true);
		}
	}
	SelectedSlot = LoadSlots[Slot];
}

void UMVVM_LoadScreen::ConfirmButtonPressed()
{
	if (IsValid(SelectedSlot))
	{
		AAuraGameModeBase::DeleteSlot(SelectedSlot->GetLoadSlotName(), SelectedSlot->SlotIndex);

		// 슬롯 초기화
		SelectedSlot->SlotStatus = Vacant;
		SelectedSlot->InitializeSlot();
		SelectedSlot->EnableSelectSlotButton.Broadcast(true);
	}
}

void UMVVM_LoadScreen::PlayButtonPressed()
{
	if (!SelectedSlot)
		return;
	
	// 멀티로 연결하던 시도가 있었다면 취소
	CancelMultiPlay();
	
	if (UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
	{
		AuraGameInstance->PlayerStartTag = SelectedSlot->PlayerStartTag;
		AuraGameInstance->LoadSlotName = SelectedSlot->GetLoadSlotName();
		AuraGameInstance->LoadSlotIndex = SelectedSlot->SlotIndex;
		
		//AuraGameInstance->HostSession(SelectedSlot->GetMapName());
	}
	
	FString LoadSlotName = SelectedSlot->GetLoadSlotName();
	FName MapName = *SelectedSlot->GetMapName();
	AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>();
	if (AuraGM)
	{
		AuraGM->TravelToMap(SelectedSlot->GetMapName());
		return;
	}
	
	UGameplayStatics::OpenLevel(GetWorld(), MapName, true, TEXT("listen"));
}

void UMVVM_LoadScreen::PlayMultiplayerButtonPressed()
{
	CancelMultiPlay();
	
	if (UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
	{
		AuraGameInstance->PlayerStartTag = SelectedSlot->PlayerStartTag;
		AuraGameInstance->LoadSlotName = SelectedSlot->GetLoadSlotName();
		AuraGameInstance->LoadSlotIndex = SelectedSlot->SlotIndex;
		
		//AuraGameInstance->FindSession();
	}
	
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		PC->ClientTravel("127.0.0.1", TRAVEL_Relative);
	}
}

void UMVVM_LoadScreen::CancelMultiPlay()
{
	if (GEngine)
	{
		GEngine->CancelPending(GetWorld());
	}
	
	if (UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(UGameplayStatics::GetGameInstance(GetWorld())))
	{
		AuraGameInstance->CancelFindSession();
	}
}

void UMVVM_LoadScreen::TutorialButtonPressed()
{
	CancelMultiPlay();
	
	if (AAuraGameModeBase* AuraGameMode = Cast<AAuraGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		if (UAuraGameInstance* AuraGameInstance = Cast<UAuraGameInstance>(AuraGameMode->GetGameInstance()))
		{
			// 디버그 멤버 변수들 초기화
			AuraGameInstance->SetAllVariablesToDefault();

			// 슬롯 선택 취소
			AuraGameInstance->PlayerStartTag = FName();
			AuraGameInstance->LoadSlotName = FString();
			AuraGameInstance->LoadSlotIndex = 0;
			SelectedSlot = nullptr;

			UGameplayStatics::OpenLevel(GetWorld(), TEXT("Tutorial"), true);
		}
	}
}

void UMVVM_LoadScreen::LoadData()
{
	UAuraGameInstance* AuraGI = GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (!IsValid(AuraGI))
		return;
	
	for (const TPair<int32, UMVVM_LoadSlot*> LoadSlot : LoadSlots)
	{
		// 저장된 게임 찾아오기
		ULoadScreenSaveGame* SaveObject = AuraGI->GetSaveSlotData(LoadSlot.Value->GetLoadSlotName(), LoadSlot.Key);

		// 플레이어 이름 가져오기
		const FString PlayerName = SaveObject->PlayerName;

		TEnumAsByte<ESaveSlotStatus> SaveSlotStatus = SaveObject->SaveSlotStatus;

		LoadSlot.Value->SlotStatus = SaveSlotStatus;
		LoadSlot.Value->SetPlayerName(PlayerName);
		LoadSlot.Value->InitializeSlot();
		
		LoadSlot.Value->SetMapName(SaveObject->MapName);
		LoadSlot.Value->PlayerStartTag = SaveObject->PlayerStartTag;
		LoadSlot.Value->SetLevel(SaveObject->PlayerLevel);
	}
}

void UMVVM_LoadScreen::SetNumLoadSlots(int32 InNum)
{
	UE_MVVM_SET_PROPERTY_VALUE(NumLoadSlots, InNum);
}
