// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HighlightInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UHighlightInterface : public UInterface
{
	GENERATED_BODY()
};


class AURA_API IHighlightInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void HighlightActor();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void UnHighlightActor();

	// Move To 컴포넌트로 이동시키는 함수!!
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void SetMoveToLocation(FVector& OutDestination);
};
