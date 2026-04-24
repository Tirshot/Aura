// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "Game/AuraGameInstance.h"
#include "UI/Widget/AuraUserWidget.h"
#include "MVVM_LoadScreen.generated.h"

class USaveGame;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSlotSelected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNetworkMessageReceived, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionsFound, FAuraSessionInfo, FoundSessions, int32, Index);

class UMVVM_LoadSlot;

UCLASS()
class AURA_API UMVVM_LoadScreen : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	void InitializeLoadSlots();
	
	UFUNCTION(BlueprintImplementableEvent)
	UAuraUserWidget* CreatePopupWidget(FString& Str);
	
public:

	UFUNCTION(BlueprintPure)
	UMVVM_LoadSlot* GetLoadSlotViewModelByIndex(int32 Index);

	// EnterName 슬롯
	UFUNCTION(BlueprintCallable)
	void NewSlotButtonPressed(int32 Slot, const FString& EnteredName);

	// Vacant 슬롯 - 새 게임 생성
	UFUNCTION(BlueprintCallable)
	void NewGameButtonPressed(int32 Slot);

	// Taken 슬롯 - 저장된 게임
	UFUNCTION(BlueprintCallable)
	void SelectSlotButtonPressed(int32 Slot);

	// 재확인창 확인 버튼
	UFUNCTION(BlueprintCallable)
	void ConfirmButtonPressed();

	// 게임 시작 버튼
	UFUNCTION(BlueprintCallable)
	void PlayButtonPressed();

	// 게임 시작 버튼
	UFUNCTION(BlueprintCallable)
	void PlayMultiplayerButtonPressed();
	
	UFUNCTION(BlueprintCallable)
	void CancelMultiPlay();
	
	UFUNCTION(BlueprintCallable)
	void TutorialButtonPressed();
	
	void LoadData();

	void SetNumLoadSlots(int32 InNum);

	int32 GetNumLoadSlots() const { return NumLoadSlots;}

public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UMVVM_LoadSlot> LoadSlotViewModelClass;

	UPROPERTY(BlueprintAssignable)
	FSlotSelected SlotSelected;
	
	UPROPERTY(BlueprintAssignable)
	FNetworkMessageReceived NetworkErrorReceived;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FNetworkMessageReceived NetworkMessageReceived;
	
	UPROPERTY(BlueprintAssignable)
	FOnSessionsFound OnSessionsFound;
	
protected:
	UPROPERTY()
	TMap<int32, UMVVM_LoadSlot*> LoadSlots;

	UPROPERTY()
	TObjectPtr<UMVVM_LoadSlot> LoadSlot_0;

	UPROPERTY()
	TObjectPtr<UMVVM_LoadSlot> LoadSlot_1;

	UPROPERTY()
	TObjectPtr<UMVVM_LoadSlot> LoadSlot_2;

	UPROPERTY()
	UMVVM_LoadSlot* SelectedSlot;
	
	UPROPERTY(BlueprintReadOnly)
	int32 SelectedSlotIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	int32 NumLoadSlots;
};
