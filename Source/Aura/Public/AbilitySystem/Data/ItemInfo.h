// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilityUpgradeInfo.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Player/CharmInstance.h"
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

	UPROPERTY(EditAnywhere, meta =(Category = "Status|Main"))
	float Strength = 0.f;

	UPROPERTY(EditAnywhere, meta =(Category = "Status|Main"))
	float Intelligence = 0.f;

	UPROPERTY(EditAnywhere, meta =(Category = "Status|Main"))
	float Resilience = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Main"))
	float Vigor = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Item"))
	float MagicAttackPower = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Item"))
	float Armor = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Item"))
	float MaxHealth = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Item"))
	float MaxMana = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Item"))
	float HealthRegeneration = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Item"))
	float ManaRegeneration = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Item"))
	float MovementSpeed = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Special"))
	float CriticalHitChance = 0.f;
	
	UPROPERTY(EditAnywhere, meta =(Category = "Status|Special"))
	float ArmorPenetration = 0.f;
	
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
	int32 AbilityLevel = 1;
	
};

USTRUCT(BlueprintType)
struct FEffectAndStack
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;
	
	UPROPERTY(EditDefaultsOnly)
	int32 EffectStack = 1;
	
};

/* 아이템 정보 구조체 */
USTRUCT(BlueprintType)
struct FItemData: public FTableRowBase
{
	GENERATED_BODY()
	
	// Item GUID Object <- 고유 이펙트 소스가 됨
	UPROPERTY(Transient)
	UObject* EffectSourceObject;

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
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item Data")
	int32 ItemCounts = 1;
	
	// 인벤토리 내의 아이콘
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	UTexture2D* Image = nullptr;
	
	// 드롭 아이템 스태틱 메시
	// 공통
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	TObjectPtr<UStaticMesh> StaticMesh = nullptr;

	// 부츠 전용
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="ItemSubGroup==EItemSubGroup::Boots", EditConditionHides),  Category = "Item Data")
	TObjectPtr<UStaticMesh> LeftFootMesh = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(EditCondition="ItemSubGroup==EItemSubGroup::Boots", EditConditionHides),  Category = "Item Data")
	TObjectPtr<UStaticMesh> RightFootMesh = nullptr;
	
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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EUpgradeRarity Rarity = EUpgradeRarity::Common;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EItemGroup ItemGroup = EItemGroup::ETC;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Data")
	EItemSubGroup ItemSubGroup = EItemSubGroup::None;
	
	bool operator==(const FItemData& Other) const
	{
		if (Name == Other.Name)
			return true;
		
		return false;
	};
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

USTRUCT(BlueprintType)
struct FDropItemGroup: public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EItemGroup ItemGroup;

	// 아이템 그룹 별 - 소모품, 장비, 기타 등 등장 확률
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Drop")
	float GroupProbability;

	// 각 아이템 별 드랍 확률
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FDropItemProbability> Items;
	
	bool IsValid() const
	{
		return Items.Num() > 0;
	}
};

USTRUCT(BlueprintType)
struct FDropItemGroupArray: public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 DropCounts = 1;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FDropItemGroup> Groups;
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

	const FDropItemGroupArray* GetDropItemGroup(ECharacterClass EnemyClass);
	
public:
	// 몬스터 클래스 별 & 각 아이템 그룹 별 + 각 아이템 별 등장 확률
	UPROPERTY(EditDefaultsOnly, Category="Item|Drop")
	TMap<ECharacterClass, FDropItemGroupArray> DropList;
};
