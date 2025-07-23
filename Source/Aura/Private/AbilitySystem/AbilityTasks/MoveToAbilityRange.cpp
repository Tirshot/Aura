// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AbilityTasks/MoveToAbilityRange.h"

#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Character/AuraCharacter.h"
#include "Components/SplineComponent.h"
#include "Interaction/HighlightInterface.h"
#include "Player/AuraPlayerController.h"

UMoveToAbilityRange::UMoveToAbilityRange()
{
	bTickingTask = true;
}

UMoveToAbilityRange* UMoveToAbilityRange::MoveToAbilityRange(UGameplayAbility* OwningAbility, const FVector TargetLocation,
	float AbilityRange, AActor* AvatarActor, AActor* TargetActor)
{
	const FName TaskInstanceName = FName("MoveToRangeInstance");
	UMoveToAbilityRange* MyTask = NewAbilityTask<UMoveToAbilityRange>(OwningAbility, TaskInstanceName);
	MyTask->TargetLocation = TargetLocation;
	MyTask->AbilityRange = AbilityRange;
	MyTask->AvatarActor = AvatarActor;
	MyTask->TargetActor = TargetActor;
	return MyTask;
}

void UMoveToAbilityRange::TickTask(float DeltaTime)
{
	Super::TickTask(DeltaTime);

	RangeCheck();
}

void UMoveToAbilityRange::RangeCheck()
{
	if (!AvatarActor.IsValid() || !Ability || !TargetActor.IsValid())
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}

	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(AvatarActor);
	if (!AuraCharacter)
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}

	AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(AuraCharacter->GetController());
	if (!AuraPC)
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}
	
	auto Spline = AuraPC->GetMoveSpline();
	FVector CurrentLocation = AvatarActor->GetActorLocation();
	FVector TargetActorLocation = TargetActor->GetActorLocation();
	FVector Direction = TargetActorLocation - CurrentLocation;
	Direction.Normalize();
	
	float CurrentDistance = FVector::Distance(CurrentLocation, TargetActorLocation);
	
	if (CurrentDistance <= AbilityRange || AuraPC->IsShiftKeyDown())
	{
		OnReached.Broadcast();
		AuraPC->SetAutoRunning(false);
		AuraPC->SetCachedDestination(CurrentLocation);
		Spline->ClearSplinePoints();
		EndTask();
	}
	else
	{
		// 이동
		MoveToTargetLocation();
	}
}

void UMoveToAbilityRange::MoveToTargetLocation()
{
	AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(AvatarActor);
	if (!AuraCharacter)
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}

	AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(AuraCharacter->GetController());
	if (!AuraPC)
	{
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}
	
	auto Spline = AuraPC->GetMoveSpline();
	FVector CurrentLocation = AvatarActor->GetActorLocation();
	FVector TargetActorLocation = TargetActor->GetActorLocation();
	FVector Direction = TargetActorLocation - CurrentLocation;
	Direction.Normalize();
	
	float CurrentDistance = FVector::Distance(CurrentLocation, TargetActorLocation);

	// 사거리까지 이동하기
	FVector MoveLocation = CurrentLocation + (Direction * FMath::Abs(CurrentDistance - AbilityRange));
        
	if (AuraCharacter->Implements<UHighlightInterface>())
	{
		IHighlightInterface::Execute_SetMoveToLocation(AuraCharacter, MoveLocation);    
	}
            
	if (UNavigationPath* NavPath = UNavigationSystemV1::FindPathToLocationSynchronously(this, CurrentLocation, MoveLocation))
	{
		// 경로가 유효하고 최소한 하나의 경로 점이 있을 때
		if (NavPath->PathPoints.Num() > 0)
		{
			// 각 경로 점을 스플라인에 추가
			Spline->ClearSplinePoints();
			for (const FVector& PointLoc : NavPath->PathPoints)
			{
				Spline->AddSplinePoint(PointLoc, ESplineCoordinateSpace::World);
			}
			AuraPC->SetAutoRunning(true);
		}
		else
		{
			AuraPC->SetAutoRunning(false);
			OnMoveFailed.Broadcast();
			EndTask();
			return;
		}
	}
	else
	{
		AuraPC->SetAutoRunning(false);
		OnMoveFailed.Broadcast();
		EndTask();
		return;
	}
}
