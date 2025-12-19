// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MessageInterface.generated.h"

UINTERFACE(MinimalAPI)
class UMessageInterface : public UInterface
{
	GENERATED_BODY()
};

class AURA_API IMessageInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
	void SetMessage(const FText& Message, UTexture2D* Image);
};
