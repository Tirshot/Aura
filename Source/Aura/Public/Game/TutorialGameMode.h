// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/TutorialData.h"
#include "Game/AuraGameModeBase.h"
#include "TutorialGameMode.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTaskCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueEnded, int32, CurrentTutorialIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialStepChanged, const FTutorialDialogueEntry&, NewEntry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRequireCountChanged, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTutorialIndexChanged, int32, NewCount);


UCLASS()
class AURA_API ATutorialGameMode : public AAuraGameModeBase
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	
	// HUD가 생성되었을 때 호출될 함수 (델리게이트 바인딩)
	virtual void PostLogin(APlayerController* NewPlayer) override;

	void TryStartTutorial();

public:
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnTutorialStepChanged TutorialStepChangedDelegate;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnDialogueEnded DialogueEnded;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnTaskCompleted TaskCompletedDelegate;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnRequireCountChanged RequireCountChangedDelegate;

	UPROPERTY(BlueprintAssignable, BlueprintCallable)
	FOnTutorialIndexChanged TutorialIndexChangedDelegate;

public:
	void OnTutorialDataLoaded();
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Tutorial")
	UTutorialData* TutorialDataAsset;

	UPROPERTY(BlueprintReadWrite, Category = "Tutorial")
	int32 CurrentTutorialIndex = 0;


protected:
	bool bIsPlayerLoggedIn = false;
	bool bTutorialDataSet = false;
};
