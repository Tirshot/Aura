// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "TutorialData.generated.h"

UENUM()
enum EDialogueAlign
{
	Left,
	Right,
};

USTRUCT(BlueprintType)
struct FDialogueLine
{
	GENERATED_BODY()

	FDialogueLine() : Alignment(EDialogueAlign::Left) {}

	// 대화문 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(MultiLine = true))
	FText Text;

	// 대화문 정렬
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TEnumAsByte<EDialogueAlign> Alignment;
};

USTRUCT(BlueprintType)
struct FTutorialDialogueEntry
{
	GENERATED_BODY()

	// 제목
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Title;
	
	// 대화문
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(MultiLine = true))
	TArray<FDialogueLine> Dialogues;

	// 태그 기반 조건
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGameplayTag> RequirementTags;
	
	// 조건 설명 텍스트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(MultiLine = true))
	FText RequirementText;

	// 조건
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 RequireCount = 0;
	
	// 조건
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TEnumAsByte<EDialogueAlign> DialogueAlign = EDialogueAlign::Left;
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
