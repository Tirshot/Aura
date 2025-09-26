// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Data/TutorialData.h"

const FTutorialDialogueEntry& UTutorialData::GetTutorialDialogueEntryByIndex(int32 Index)
{
	FTutorialDialogueEntry DummyEntry = FTutorialDialogueEntry();

	if (TutorialDialogueEntries.IsEmpty())
		return DummyEntry;
	
	const int32 MaxIndex = TutorialDialogueEntries.Num();
	const int32 ClampedIndex = FMath::Clamp(Index, 0, MaxIndex - 1);
	
	return TutorialDialogueEntries[ClampedIndex];
}
