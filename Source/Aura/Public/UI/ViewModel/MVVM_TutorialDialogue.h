// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MVVM_TutorialDialogue.generated.h"

UCLASS()
class AURA_API UMVVM_TutorialDialogue : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void BlueprintInitialize();
	
	FText GetCurrentDialogue() const {return CurrentDialogue;}
	int32 GetCurrentDialogueIndex() const {return CurrentDialogueIndex;}
	FText GetRequirementText() const {return RequirementText;}
	int32 GetRequireCount() const {return RequireCount;}
	int32 GetCurrentCount() const {return CurrentCount;}

	void SetCurrentDialogue(FText InDialogue);
	void SetCurrentDialogueIndex(int32 InIndex);
	void SetRequirementText(FText InText);
	void SetRequireCount(int32 InCount);
	void SetCurrentCount(int32 InCount);

	void SetViewToViewModel(UUserWidget* View) { TutorialDialogueView = View; }

	UFUNCTION(BlueprintCallable)
	UUserWidget* GetView(){return TutorialDialogueView;}
	
	UFUNCTION(BlueprintCallable)
	void SetViewPosition(const FVector2D& Location);

public:
	// 다이얼로그 진행
	UFUNCTION(BlueprintCallable)
	void GoToNextDialogue();

protected:
	UPROPERTY()
	TObjectPtr<UUserWidget> TutorialDialogueView;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta =(AllowPrivateAccess="true"))
	TArray<FText> DialoguesArray;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	FText CurrentDialogue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	int32 CurrentDialogueIndex = 0;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	FText RequirementText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	int32 CurrentCount;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, FieldNotify, Setter, Getter, meta =(AllowPrivateAccess="true"))
	int32 RequireCount;
};
