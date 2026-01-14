// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharmInstance.Generated.h"

UCLASS()
class AURA_API UCharmInstance : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FGuid ItemID;
};
