// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AuraGameModeBase.generated.h"

class ULoadScreenSaveGame;
class UCharacterClassInfo;
class UAbilityInfo;
class UMVVM_LoadSlot;
class USaveGame;

UCLASS()
class AURA_API AAuraGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;

public:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

public:
	void SaveSlotData(UMVVM_LoadSlot* LoadSlot, int32 SlotIndex);
	static void DeleteSlot(const FString& SlotName, int32 SlotIndex);
	ULoadScreenSaveGame* RetrieveInGameSaveData();
	void SaveInGameProgressData(ULoadScreenSaveGame* SaveObject);

	void TravelToMap(UMVVM_LoadSlot* Slot);

	ULoadScreenSaveGame* GetSaveSlotData(const FString& SlotName, int32 SlotIndex) const;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USaveGame> LoadScreenSaveGameClass;


public:
	UPROPERTY(EditDefaultsOnly, Category="Character Class Default")
	TObjectPtr<UCharacterClassInfo> CharacterClassInfo;

	UPROPERTY(EditDefaultsOnly, Category="Ability Info")
	TObjectPtr<UAbilityInfo> AbilityInfo;

	// �� �̸� ����
	UPROPERTY(EditDefaultsOnly)
	FString DefaultMapName;

	// Soft Object Ptr : �ε�Ǳ� ������ �޸𸮿� ������� ����
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> DefaultMap;

	UPROPERTY(EditDefaultsOnly)
	FName DefaultPlayerStartTag;

	UPROPERTY(EditDefaultsOnly)
	TMap<FString, TSoftObjectPtr<UWorld>> Maps;
};
