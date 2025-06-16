// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "MVVM_GameOver.generated.h"

/**
 * 
 */
UCLASS()
class AURA_API UMVVM_GameOver : public UMVVMViewModelBase
{
	GENERATED_BODY()

public:

	void BindCallbacksToDependencies();
	
};
