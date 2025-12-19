// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityUpgradeInfo.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ItemInfo.generated.h"

enum class ECharacterClass : uint8;
class AAuraEnemy;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EItemGroup : uint8
{
	Usable = 0,
	Equipment = 1,
	Charm = 2,
	ETC = 3
};

UENUM(BlueprintType)
enum class EItemSubGroup : uint8
{
	Helmet = 0,
	Armor = 1,
	Boots = 2,
	Weapon = 3,
	None = 4
};

USTRUCT(BlueprintType)
struct FItemStat
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float Strength = 0.f;

	UPROPERTY(EditAnywhere)
	float Intelligence = 0.f;

	UPROPERTY(EditAnywhere)
	float Resilience = 0.f;
	
	UPROPERTY(EditAnywhere)
	float Vigor = 0.f;
	
	UPROPERTY(EditAnywhere)
	float MovementSpeed = 0.f;
	
	UPROPERTY(EditAnywhere)
	float MaxHealth = 0.f;
	
	UPROPERTY(EditAnywhere)
	float MaxMana = 0.f;
	
	bool IsEmpty() const { return Strength == 0.f && Intelligence == 0.f && Resilience == 0.f && Vigor == 0.f && MovementSpeed == 0.f && MaxHealth == 0.f && MaxMana == 0.f;}
};

USTRUCT(BlueprintType)
struct FAbilityTagAndLevel
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag AbilityTag = FGameplayTag::EmptyTag;
	
	UPROPERTY(EditDefaultsOnly)
	int32 AbilityLevel = 0;
	
};

USTRUCT(BlueprintType)
struct FEffectAndStack
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;
	
	UPROPERTY(EditDefaultsOnly)
	int32 EffectStack = 0;
	
};

/* 아이템 정보 구조체 */
USTRUCT(BlueprintType)
struct FItemData: public FTableRowBase
{
	GENERATED_BODY()

	// !!각 행의 이름을 ItemID와 일치시켜야 함
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FName Name = FName();
	
	// 게임 내 표시되는 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FText DisplayName = FText();
	
	// 인벤토리에서 차지하는 격자 크기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FIntPoint Size = FIntPoint(1, 1);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	bool bStackable = false;
	
	// 인벤토리 내의 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	UTexture2D* Image = nullptr;
	
	// 드롭 아이템 스태틱 메시
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	UStaticMesh* StaticMesh = nullptr;
	
	// 드롭 아이템 머티리얼
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	UMaterialInterface* Material = nullptr;
	
	// 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true), Category = "Item Data")
	FText Description = FText();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	FItemStat ItemStat = FItemStat();
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> ItemStatEffectClass;
	
	// 게임플레이 이펙트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	TArray<FEffectAndStack> EffectAndStacks;
	
	// 부여할 어빌리티의 태그들
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	TArray<FAbilityTagAndLevel> AbilityTagAndLevel;
	
	// 부여할 어빌리티 업그래이드들
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	TArray<FAbilityTagAndLevel> AbilityUpgradeTagAndLevel;
	
	// 효과 설명
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true), Category = "Item Data")
	FText EffectDescription = FText();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EUpgradeRarity Rarity = EUpgradeRarity::Common;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EItemGroup ItemGroup = EItemGroup::ETC;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EItemSubGroup ItemSubGroup = EItemSubGroup::None;
};

/* 아이템 드롭 확률 */
USTRUCT(BlueprintType)
struct FDropItemProbability: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FDataTableRowHandle ItemHandle;

	// 드롭 확률 (0~100)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float DropProbability = 10.f;
};

// 드롭 확률 구조체를 TArray로 래핑
USTRUCT(BlueprintType)
struct FDropItemProbabilityArray: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 DropItemCounts = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FDropItemProbability> DropItemProbabilities;
};

UCLASS()
class AURA_API UItemInfo : public UDataAsset
{
	GENERATED_BODY()

public:
	// 아이템 정보
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	TObjectPtr<UDataTable> ItemTable;
	
	UFUNCTION(BlueprintCallable, Category = "Item")
	FItemData GetItemDataByID_Copy(const FName& ItemID) const;
	
	const FItemData* GetItemDataByID(const FName& ItemID) const;

public:
	// 아이템 드랍 확률
	// 아이템 그룹 별 - 소모품, 장비, 기타 등 등장 확률
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Drop")
	TMap<EItemGroup, float> ItemGroupDropProbability;

	// 몬스터 클래스 별 & 각 아이템 별 등장 확률
	UPROPERTY(EditDefaultsOnly, Category="Item|Drop")
	TMap<ECharacterClass, FDropItemProbabilityArray> DropList;
};
