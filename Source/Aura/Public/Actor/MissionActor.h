// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "AbilitySystem/Data/MissionInfo.h"
#include "GameFramework/Actor.h"
#include "Player/AuraPlayerController.h"
#include "MissionActor.generated.h"

class ULevelSequence;
class ALevelSequenceActor;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndMission);

// 이 액터가 레벨에 배치되면 미션을 발동시킴
UCLASS()
class AURA_API AMissionActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AMissionActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void StartMission();

	UFUNCTION(BlueprintCallable, Category = "Mission")
	void EndMission(bool bSucceed);
	
	UPROPERTY(BlueprintAssignable)
	FOnEndMission OnEndMissionDelegate;
	
public:
	// 연출
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCinematic();
	
	UFUNCTION()
	void OnCinematicFinished();
	
public:
	/* 
	 *  미션 수행 관련 함수
	 */
	void CalculateMultiplier(int32 PlayerCounts);
	void ApplyMissionEffectToPC(AAuraPlayerController* JoinedPC);
	
	// 데미지를 보고받음
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void ReportValue(const FGameplayTag& ObjectiveTag, float Value);
	
	// 미션 달성 체크
	void CheckObjectiveComplete();
	
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void GiveRewards();
	
	UFUNCTION(BlueprintCallable, Category = "Mission")
	void FinishAndDestroy();

public:
	// 콜백 함수
	UFUNCTION()
	void OnPlayerCountUpdated(int32 PlayerCounts, AAuraPlayerController* JoinedPC);

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Mission")
	TObjectPtr<UMissionInfo> MissionInfo;

	UPROPERTY()
	FMissionData DynamicMissionData;
	
	UPROPERTY(EditAnywhere, Category = "Mission")
	TObjectPtr<ULevelSequence> MissionSequenceAsset;

	UPROPERTY(EditInstanceOnly, Category = "Mission")
	bool bEndMissionEarly = false;

	FTimerHandle MissionTimerHandle;
	
	UPROPERTY()
	FGameplayTagContainer ActiveEffectTags;

	void UpdateTimer();
	
	void SyncDataToGameState();

};
