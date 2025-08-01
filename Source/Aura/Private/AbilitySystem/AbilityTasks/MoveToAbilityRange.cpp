// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/MoveToAbilityRange.h"

#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Character/AuraCharacter.h"
#include "Components/SplineComponent.h"
#include "Player/AuraPlayerController.h"

UMoveToAbilityRange::UMoveToAbilityRange()
{
	bTickingTask = true;
}

UMoveToAbilityRange* UMoveToAbilityRange::MoveToAbilityRange(UGameplayAbility* OwningAbility, const FVector TargetLocation,
	float AbilityRange, AActor* AvatarActor)
{
	const FName TaskInstanceName = FName("MoveToRangeInstance");
	UMoveToAbilityRange* MyTask = NewAbilityTask<UMoveToAbilityRange>(OwningAbility, TaskInstanceName);
	MyTask->TargetLocation = TargetLocation;
	MyTask->AbilityRange = AbilityRange;
	MyTask->AvatarActor = AvatarActor;
	return MyTask;
}

void UMoveToAbilityRange::Activate()
{
	Super::Activate();
	
	FVector CurrentLocation = AvatarActor->GetActorLocation();

	AuraCharacter = Cast<AAuraCharacter>(AvatarActor);
	if (!AuraCharacter.IsValid())
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}

	AuraPC = Cast<AAuraPlayerController>(AuraCharacter->GetController());
	if (!AuraPC.IsValid())
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}
	
	auto Spline = AuraPC->GetMoveSpline();
	
	FVector Direction = TargetLocation - CurrentLocation;
	Direction.Normalize();
	
	// 사거리까지 이동하기
	FVector MoveLocation = CurrentLocation + (Direction * AbilityRange);
        
	if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, CurrentLocation, MoveLocation))
	{
		// 경로가 유효하고 최소한 하나의 경로 점이 있을 때
		if (NavPath->PathPoints.Num() > 0)
		{
			// 각 경로 점을 스플라인에 추가
			if (Spline)
			{
				Spline->ClearSplinePoints();
				for (const FVector& PointLoc : NavPath->PathPoints)
				{
					Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
				}
			}
			AuraPC->SetCachedDestination(NavPath->PathPoints[NavPath->PathPoints.Num() - 1]);
			AuraPC->SetAutoRunning(true);
		}
		else
		{
			OnMoveFailed.Broadcast();
			EndTask();
			return;
		}
	}
	else
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}
}

void UMoveToAbilityRange::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	RangeCheck();
}

void UMoveToAbilityRange::OnDestroy(bool bInOwnerFinished)
{
	if (AuraPC.IsValid())
	{
		AuraPC->StopAutoRun();

		auto Spline = AuraPC->GetMoveSpline();
		if (Spline)
		{
			Spline->ClearSplinePoints();
		}
	}
	
	Super::OnDestroy(bInOwnerFinished);
}

void UMoveToAbilityRange::RangeCheck()
{
	if (!AvatarActor.IsValid() || !AuraCharacter.IsValid() || !AuraPC.IsValid())
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}
	
	FVector CurrentLocation = AvatarActor->GetActorLocation();
	
	float CurrentDistance = FVector::Distance(CurrentLocation, TargetLocation);
	
	if (CurrentDistance <= AbilityRange || AuraPC->IsShiftKeyDown())
	{
		OnReached.Broadcast();
		EndTask();
	}

	AuraCharacter = Cast<AAuraCharacter>(AvatarActor);
	if (!AuraCharacter.IsValid())
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}

	AuraPC = Cast<AAuraPlayerController>(AuraCharacter->GetController());
	if (!AuraPC.IsValid())
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}
}
