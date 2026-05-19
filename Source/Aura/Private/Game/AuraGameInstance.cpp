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
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Components/MenuAnchor.h"
#include "Online/OnlineSessionNames.h"
#include "Player/CharmComponent.h"
#include "UI/Widget/LoadScreenWidget.h"
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
	
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UAuraGameInstance::OnPreLoadMap);
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

void UAuraGameInstance::OnPreLoadMap(const FString& MapName)
{
	// 서버에서 모든 플레이어의 비율 저장
	if (!GetWorld())
		return;
	
	if (GetWorld()->GetNetMode() == NM_Client)
		return;
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC)
			continue;

		AAuraPlayerState* AuraPS = PC->GetPlayerState<AAuraPlayerState>();
		if (!AuraPS || !AuraPS->Charm) 
			continue;

		UAuraAttributeSet* AS = Cast<UAuraAttributeSet>(AuraPS->GetAttributeSet());
		if (!AS)
			continue;

		float MaxHP = AS->GetMaxHealth();
		float HP = AS->GetHealth();
		float MaxMP = AS->GetMaxMana();
		float MP = AS->GetMana();

		AuraPS->Charm->SavedHPRatio = (MaxHP > 0.f) ? (HP / MaxHP) : 1.f;
		AuraPS->Charm->SavedMPRatio = (MaxMP > 0.f) ? (MP / MaxMP) : 1.f;
		AuraPS->Charm->bHasSavedRatio = true;
	}
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
			SessionInterface->DestroySession(NAME_GameSession);
	
			FOnlineSessionSettings SessionSettings;
			SessionSettings.bIsLANMatch = true;
			SessionSettings.NumPublicConnections = 4;
			SessionSettings.bShouldAdvertise = true;
			SessionSettings.bUsesPresence = false;
			SessionSettings.bAllowJoinInProgress = true;
			
			LoadMapName = MapName;

			// 델리게이트 바인딩
			CreateSessionCompleteDelegateHandle = SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UAuraGameInstance::OnCreateSessionComplete);

			if (!SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings))
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
			SessionSearch->MaxSearchResults = 100;
			//SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

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
			// 세션 제거 델리게이트
			DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
				FOnDestroySessionCompleteDelegate::CreateUObject(this, &UAuraGameInstance::OnDestroySessionComplete)
			);
			
			// 기존 세션 제거
			SessionInterface->DestroySession(NAME_GameSession);
		}
	}
	// 세션이 없으면 즉시 맵 전환
	OnDestroySessionComplete(NAME_GameSession, false);
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
	
		// IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
		// if (SessionInterface.IsValid())
		// {
		// 	FNamedOnlineSession* ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
		// 	if (ExistingSession)
		// 	{
		// 		SessionInterface->DestroySession(NAME_GameSession);
		// 	}
		// }
	}
}

void UAuraGameInstance::JoinSelectedSession(int32 Index)
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem || !LastSearchSession.IsValid())
		return;

	IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
	if (!SessionInterface.IsValid() || !LastSearchSession->SearchResults.IsValidIndex(Index))
		return;

	JoinSessionCompleteDelegateHandle = SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UAuraGameInstance::OnJoinSessionComplete);

	SessionInterface->JoinSession(0, NAME_GameSession, LastSearchSession->SearchResults[Index]);
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
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
	{
		// AreYouSure UI 제거
		if (auto LoadHUD = PC->GetHUD<ALoadScreenHUD>())
		{
			LoadHUD->OnSessionFound.Broadcast();
		}
		
		// 세션 목록 UI
		if (IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
		{
			IOnlineSessionPtr SessionInterface = Subsystem->GetSessionInterface();
			if (SessionInterface.IsValid())
				SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);

			if (bWasSuccessful && SessionSearch.IsValid())
			{
				LastSearchSession = SessionSearch; // 나중을 위해 저장
        
				TArray<FAuraSessionInfo> FoundSessions;
				for (int32 i = 0; i < SessionSearch->SearchResults.Num(); ++i)
				{
					const FOnlineSessionSearchResult& Result = SessionSearch->SearchResults[i];
					FAuraSessionInfo Info;
            
					Info.ServerName = Result.Session.OwningUserName;
					Info.CurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
					Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
					Info.Ping = Result.PingInMs;
					Info.SearchResultIndex = i;

					FoundSessions.Add(Info);
				
					if (ALoadScreenHUD* LoadHUD = PC->GetHUD<ALoadScreenHUD>())
					{
						LoadHUD->LoadScreenViewModel->OnSessionsFound.Broadcast(Info, i);
					}
				}
				UE_LOG(LogTemp, Warning, TEXT(" Found Session Num : %d "), FoundSessions.Num())
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
				UE_LOG(LogTemp, Warning, TEXT("Raw ConnectString: %s"), *Address);

				if (APlayerController* PlayerController = GetFirstLocalPlayerController())
				{
					bIsOnline = true;
					
					// Address에 맵 경로가 붙음???
					if (Address.Contains(TEXT("/")))
					{
						// 슬래시 이후 제거
						int32 SlashIndex;
						Address.FindChar('/', SlashIndex);
						Address = Address.Left(SlashIndex);
					}
					
					// IP만 추출
					FString IPOnly;
					FString PortStr;
					Address.Split(TEXT(":"), &IPOnly, &PortStr);

					// 포트 0이면 6112로
					int32 Port = FCString::Atoi(*PortStr);
					if (Port <= 0)
						Port = 6112;

					FString TravelURL = FString::Printf(TEXT("%s:%d"), *IPOnly, Port);
            
					UE_LOG(LogTemp, Warning, TEXT("Final Travel URL: %s"), *TravelURL);

					PlayerController->ClientTravel(
						FString::Printf(TEXT("%s"), *TravelURL),
						ETravelType::TRAVEL_Absolute,
						false);
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
		
		APlayerController* PC = GetFirstLocalPlayerController();
		if (PC)
		{
			// 메인 메뉴 레벨로 안전하게 이동시키기 (기존 접속 끊기)
			PC->ClientTravel("/Game/Maps/MainMenu", ETravelType::TRAVEL_Absolute);
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
