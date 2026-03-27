// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/AuraGameInstance.h"
#include "Engine/Engine.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "Game/LoadScreenSaveGame.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/GameplayStatics.h"
#include "UI/HUD/LoadScreenHUD.h"
#include "UI/ViewModel/MVVM_LoadScreen.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "SocketSubsystem.h"
#include "Online/OnlineSessionNames.h"
#include "UI/WidgetController/SettingsMenuWidgetController.h"

void UAuraGameInstance::Init()
{
	// 네트워크 어댑터 검색
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (SocketSubsystem)
	{
		TArray<TSharedPtr<FInternetAddr>> AdapterAddresses;
		if (SocketSubsystem->GetLocalAdapterAddresses(AdapterAddresses))
		{
			FString TargetIP = "";

			for (const auto& Address : AdapterAddresses)
			{
				FString CurrentIP = Address->ToString(false);
                
				// 라드민 IP 검색
				if (CurrentIP.StartsWith(TEXT("26.")))
				{
					TargetIP = CurrentIP;
					UE_LOG(LogTemp, Warning, TEXT("Radmin Adapter Found: %s"), *TargetIP);
					break;
				}
			}

			// MultiHome 강제 주입
			if (!TargetIP.IsEmpty())
			{
				// 이미 설정된 MultiHome이 없을 때만 추가
				if (!FParse::Param(FCommandLine::Get(), TEXT("MultiHome")))
				{
					FCommandLine::Append(*FString::Printf(TEXT(" -MultiHome=%s"), *TargetIP));
					UE_LOG(LogTemp, Warning, TEXT("Successfully forced MultiHome to Radmin IP: %s"), *TargetIP);
				}
			}
		}
	}
	Super::Init();

	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	if (Settings)
	{
		Settings->LoadSettings(false);
		Settings->ApplySettings(false);
	}

	// 모든 Aura Character 대상으로 인벤토리 컴포넌트의 델리게이트 호출
	if (ItemInfos)
	{
		OnInitialized.Broadcast();
		bInit = true;
	}
	
	// 네트워크 접속 실패 시 바인딩
	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UAuraGameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UAuraGameInstance::HandleTravelFailure);
	}
}

void UAuraGameInstance::Shutdown()
{
	if (auto AuraASC = Cast<UAuraAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetFirstLocalPlayerController())))
	{
		FGameplayEffectQuery Query;
		AuraASC->RemoveActiveEffects(Query);
	}
	
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		// 세션 종료
		if (SessionInterface.IsValid())
		{
			DestroySessionCompleteDelegateHandle = SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UAuraGameInstance::OnDestroySessionComplete);
        
			SessionInterface->DestroySession(NAME_GameSession);
		}
	}
	
	Super::Shutdown();
}

ULoadScreenSaveGame* UAuraGameInstance::GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const
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

USettingsMenuWidgetController* UAuraGameInstance::GetSettingsMenuWidgetController()
{
	if (SettingsMenuWidgetController == nullptr)
	{   // 없으면 생성
		SettingsMenuWidgetController = NewObject<USettingsMenuWidgetController>(this, SettingsMenuWidgetControllerClass);
		SettingsMenuWidgetController->BindCallbacksToDependencies();
		SettingsMenuWidgetController->Initialize();
	}
	return SettingsMenuWidgetController;
}

void UAuraGameInstance::HostSession(FString MapName)
{
	// 세션 생성
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// 기존 세션 제거
			SessionInterface->DestroySession(FName(LoadSlotName));
	
			FOnlineSessionSettings SessionSettings;
			SessionSettings.bIsLANMatch = true;
			SessionSettings.NumPublicConnections = 4;
			SessionSettings.bShouldAdvertise = true;
			SessionSettings.bUsesPresence = true;
			SessionSettings.bAllowJoinInProgress = true;
			
			LoadMapName = MapName;

			// 델리게이트 바인딩
			CreateSessionCompleteDelegateHandle = SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UAuraGameInstance::OnCreateSessionComplete);

			if (!SessionInterface->CreateSession(0, FName(LoadSlotName), SessionSettings))
			{
				// 실패 시 핸들 제거
				SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
			}
		}
	}
}

void UAuraGameInstance::FindSession()
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionSearch = MakeShareable(new FOnlineSessionSearch());
			SessionSearch->bIsLanQuery = true;
			SessionSearch->MaxSearchResults = 10;
			SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

			// 검색 완료 델리게이트 바인딩
			FindSessionsCompleteDelegateHandle = SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UAuraGameInstance::OnFindSessionsComplete);

			// 검색 시작
			SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
		}
	}
}

void UAuraGameInstance::DestroySession()
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			// 기존 세션 제거
			SessionInterface->DestroySession(FName(LoadSlotName));
		}
	}
}

void UAuraGameInstance::CancelFindSession()
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->CancelFindSessions();
		}
	}
}

void UAuraGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		}

		if (bWasSuccessful)
		{
			if (UWorld* World = GetWorld())
			{
				bIsOnline = true;
				World->ServerTravel(LoadMapName + FString("?listen"));
			}
		}
		else
		{
			bIsOnline = false;
		}
	}
}

void UAuraGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

		if (bWasSuccessful && SessionSearch.IsValid() && SessionSearch->SearchResults.Num() > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Session Found!"));

			// 접속 완료 델리게이트 바인딩
			JoinSessionCompleteDelegateHandle = SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UAuraGameInstance::OnJoinSessionComplete);

			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				if (ALoadScreenHUD* LoadHUD = PC->GetHUD<ALoadScreenHUD>())
				{
					LoadHUD->LoadScreenViewModel->NetworkMessageReceived.Broadcast(TEXT("세션을 찾았습니다.\n접속 시도 중..."));
				}
			}
			// 0번 인덱스 세션에 조인 시도
			SessionInterface->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[0]);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("No sessions found!!!"));
		
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				if (ALoadScreenHUD* LoadHUD = PC->GetHUD<ALoadScreenHUD>())
				{
					LoadHUD->LoadScreenViewModel->NetworkErrorReceived.Broadcast(TEXT("세션을 찾을 수 없습니다."));
				}
			}
		}
	}
}

void UAuraGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);

		if (Result == EOnJoinSessionCompleteResult::Success)
		{
			FString Address;
			if (SessionInterface->GetResolvedConnectString(SessionName, Address))
			{
				if (APlayerController* PlayerController = GetFirstLocalPlayerController())
				{
					bIsOnline = true;
					
					// 서버로 이동
					PlayerController->ClientTravel(Address, ETravelType::TRAVEL_Absolute);
				}
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Join Session Failed!"));
		
			if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
			{
				if (ALoadScreenHUD* LoadHUD = PC->GetHUD<ALoadScreenHUD>())
				{
					LoadHUD->LoadScreenViewModel->NetworkErrorReceived.Broadcast(TEXT("세션에 접속하지 못했습니다."));
				}
			}
		}
	}
}

void UAuraGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		}

		if (bWasSuccessful)
		{
			UE_LOG(LogTemp, Log, TEXT("Session %s Destroyed by Shutdown"), *SessionName.ToString());
		}
	}
}

void UAuraGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if (!World)
		return;
	
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ALoadScreenHUD* LoadHUD = PC->GetHUD<ALoadScreenHUD>())
		{
			LoadHUD->LoadScreenViewModel->NetworkErrorReceived.Broadcast(ErrorString);
		}
	}
}

void UAuraGameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if (!World)
		return;
	
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (ALoadScreenHUD* LoadHUD = PC->GetHUD<ALoadScreenHUD>())
		{
			LoadHUD->LoadScreenViewModel->NetworkErrorReceived.Broadcast(ErrorString);
		}
	}
}

void UAuraGameInstance::SetAllVariablesToDefault()
{
	bVisibleNextButton = false;
	bVisibleLevelUpButton = false;
	bAuraInvincible = false;
	bAuraInfiniteMana = false;
}

UItemInfo* UAuraGameInstance::GetItemInfos()
{
	if (!ItemInfos)
	{
		ItemInfos = NewObject<UItemInfo>(this, ItemInfosClass);
	}
	
	return ItemInfos;
}

const FItemData* UAuraGameInstance::GetItemData(FName ItemName)
{
	if (ItemName.IsNone()) 
		return nullptr;
	
	if (auto ItemData = GetItemInfos()->GetItemDataByID(ItemName))
	{
		return ItemData;
	}
	return nullptr;
}
