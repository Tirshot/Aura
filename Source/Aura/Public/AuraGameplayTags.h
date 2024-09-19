// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"

/**
 * 
 */
struct FAuraGameplayTags
{
	/* �̱��� �Ŵ���
	* �����Ϳ��� ��� ������ �⺻ �±�
	*/

public:
	static const FAuraGameplayTags& Get() { return GameplayTags; }
	static void InitailizeNativeGameplayTags();

	FGameplayTag Attributes_Secondary_Armor;

protected:


private:
	static FAuraGameplayTags GameplayTags;
};
