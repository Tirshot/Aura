// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MissionInfo.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class EMissionRewardType : uint8
{
	None,
	SpellPoint      UMETA(DisplayName = "스펠 포인트"),
	AttributePoint  UMETA(DisplayName = "속성 포인트"),
	Item            UMETA(DisplayName = "아이템"),
	AbilityUpgrade	UMETA(DisplayName = "어빌리티 업그레이드")
};

USTRUCT(BlueprintType)
struct FMissionReward
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EMissionRewardType RewardType = EMissionRewardType::None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Amount = 0;

	// 보상이 아이템이면 데이터테이블 행 핸들 노출
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "RewardType == EMissionRewardType::Item", EditConditionHides))
	FDataTableRowHandle ItemHandle;
};

USTRUCT(BlueprintType)
struct FMissionObjective
{
	// 각 미션 오브젝티브
	GENERATED_BODY()

	// 어떤 미션의 오브젝티브인가
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag MyMissionTag;
	
	// 오브젝티브 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ObjectiveTag;
	
	// 미션 내의 몇번째 오브젝티브인가
	UPROPERTY(BlueprintReadOnly)
	int32 ObjectiveIndex = -1;
	
	// 설명
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ObjectiveDescription;

	// 필수 달성 목표 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsRequired = false; 
	
	// 현재 값
	UPROPERTY(BlueprintReadWrite)
	float CurrentValue = 0.f;

	// 목표 값
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetValue = 0.f;
	
	// 미션 성공 시 지급할 보상 목록
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reward")
	TArray<FMissionReward> Rewards;

	// 목표 도달 여부
	UPROPERTY(BlueprintReadWrite)
	bool bIsReached = false; 
};

// 미션
USTRUCT(BlueprintType)
struct FMissionData : public FFastArraySerializerItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag MissionTag = FGameplayTag::EmptyTag;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText MissionTitle = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Multiline = true))
	FText MissionDescription = FText::GetEmpty();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Multiline = true))
	FText MissionModifiers = FText::GetEmpty();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	bool bSetValueFixed = false;

	// 제한 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TimeRemaining = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApplyOnStart;
	
	// 오브젝티브 배열
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMissionObjective> Objectives;
	
	// 미션이 시작되었는가
	UPROPERTY(BlueprintReadWrite)
	bool bIsActive = false;
	
	// 미션이 종료되었는가
	UPROPERTY(BlueprintReadWrite)
	bool bIsEnded = false;
	
	// 리플리케이션 콜백 함수
	void PostReplicatedAdd(const struct FMissionDataArray& InArraySerializer);
	void PostReplicatedChange(const struct FMissionDataArray& InArraySerializer);
};

USTRUCT(BlueprintType)
struct FMissionDataArray : public FFastArraySerializer
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FMissionData> Missions;

	UPROPERTY()
	class AAuraGameStateBase* OwningGameState = nullptr;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FMissionData, FMissionDataArray>(Missions, DeltaParms, *this);
	}
};

template<>
struct TStructOpsTypeTraits<FMissionDataArray> : public TStructOpsTypeTraitsBase2<FMissionDataArray>
{
	enum { WithNetDeltaSerializer = true };
};

UCLASS(BlueprintType)
class AURA_API UMissionInfo : public UDataAsset
{
	GENERATED_BODY()
	
public:
	const FMissionData& GetMissionData() {return MissionData;}
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FMissionData MissionData;
};
