// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ViewModel/MVVM_TutorialDialogue.h"

#include "Blueprint/UserWidget.h"
#include "Game/TutorialGameMode.h"

void UMVVM_TutorialDialogue::BlueprintInitialize_Implementation()
{
	
}

void UMVVM_TutorialDialogue::SetCurrentDialogue(FText InDialogue)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentDialogue, InDialogue);
}

void UMVVM_TutorialDialogue::SetCurrentDialogueIndex(int32 InIndex)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentDialogueIndex, InIndex);
}

void UMVVM_TutorialDialogue::SetRequirementText(FText InText)
{
	UE_MVVM_SET_PROPERTY_VALUE(RequirementText, InText);
}

void UMVVM_TutorialDialogue::SetRequireCount(int32 InCount)
{
	UE_MVVM_SET_PROPERTY_VALUE(RequireCount, InCount);
}

void UMVVM_TutorialDialogue::SetCurrentCount(int32 InCount)
{
	UE_MVVM_SET_PROPERTY_VALUE(CurrentCount, InCount);
}

void UMVVM_TutorialDialogue::SetCurrentDialogueAlignment(const EDialogueAlign& InAlignment)
{
	CurrentDialogueAlignment = InAlignment;
}

void UMVVM_TutorialDialogue::SetViewPosition(const FVector2D& Location)
{
	
}

void UMVVM_TutorialDialogue::GoToNextDialogue()
{
	// 다음 단락으로 이동
	if (DialoguesArray.IsEmpty())
		return;
	
	const int32 DialogueIndexNum = DialoguesArray.Num();
	const int32 NextDialogueIndex = FMath::Clamp(CurrentDialogueIndex + 1, 0, DialogueIndexNum - 1);

	if (CurrentDialogueIndex == NextDialogueIndex)
	{
		if (!RequirementText.IsEmpty())
			SetCurrentDialogue(RequirementText);

		// 다이얼로그 끝에 도달->게임모드 델리게이트 호출
		if (auto* TutorialGameMode = GetWorld()->GetAuthGameMode<ATutorialGameMode>())
		{
			// 몇 번째 인덱스에 있는지 전달
			TutorialGameMode->DialogueEnded.Broadcast(TutorialGameMode->CurrentTutorialIndex);
		}
	}
	else
	{
		SetCurrentDialogueIndex(NextDialogueIndex);

		const FText& NextDialogue = DialoguesArray[CurrentDialogueIndex].Text;
		SetCurrentDialogue(NextDialogue);
	}
}

void UMVVM_TutorialDialogue::GoBackToPrevDialogue()
{
	if (DialoguesArray.IsEmpty())
		return;
	
	const int32 DialogueIndexNum = DialoguesArray.Num();
	const int32 PrevDialogueIndex = FMath::Clamp(CurrentDialogueIndex - 1, 0, DialogueIndexNum - 1);

	if (CurrentDialogueIndex != DialogueIndexNum)
	{
		SetCurrentDialogueIndex(PrevDialogueIndex);

		const FText& PrevDialogue = DialoguesArray[CurrentDialogueIndex].Text;
		SetCurrentDialogue(PrevDialogue);
	}
}

