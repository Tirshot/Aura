// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"

/**
 * 
 */
struct FAuraGameplayTags
{
	/* 싱글톤 매니저
	* 에디터에서 사용 가능한 기본 태그
	*/

public:
	static const FAuraGameplayTags& Get() { return GameplayTags; }
	static void InitailizeNativeGameplayTags();

	FGameplayTag Attributes_Secondary_Armor;

protected:


private:
	static FAuraGameplayTags GameplayTags;
};
