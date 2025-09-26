// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "TutorialData.generated.h"

USTRUCT(BlueprintType)
struct FTutorialDialogueEntry
{
	GENERATED_BODY()

	// 제목
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Title;
	
	// 대화문
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(MultiLine = true))
	TArray<FText> Dialogues;

	// 태그 기반 조건
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGameplayTag> RequirementTags;
	
	// 조건 설명 텍스트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(MultiLine = true))
	FText RequirementText;

	// 조건
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RequireCount = 0;
};



UCLASS()
class AURA_API UTutorialData : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	const TArray<FTutorialDialogueEntry>& GetTutorialDialogueEntry() {return TutorialDialogueEntries;}

	UFUNCTION(BlueprintCallable)
	const FTutorialDialogueEntry& GetTutorialDialogueEntryByIndex(int32 Index);

public:
	UPROPERTY(EditDefaultsOnly)
	TArray<FTutorialDialogueEntry> TutorialDialogueEntries;
};
