// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/AuraCharacterMovementComponent.h"

#include "AbilitySystem/AuraAttributeSet.h"
#include "Character/AuraCharacter.h"
#include "GameFramework/Character.h"

void UAuraCharacterMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	
	bMaintainHorizontalGroundVelocity = true;
	bForceNextFloorCheck = false;
	bNetworkAlwaysReplicateTransformUpdateTimestamp = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	
	NetworkSimulatedSmoothLocationTime = 0.1f;  // 기본 0.083
	NetworkSimulatedSmoothRotationTime = 0.1f; // 기본 0.033
    
	NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
}

void UAuraCharacterMovementComponent::ServerAutonomousProxyTick(float DeltaTime)
{
	Super::ServerAutonomousProxyTick(DeltaTime);
	
	//매 프레임 틱마다 캐릭터 포즈 틱을 실행하게 함
	if (IsNetMode(NM_ListenServer) && CharacterOwner && CharacterOwner->GetMesh())
	{	
		USkeletalMeshComponent* CharacterMesh = CharacterOwner->GetMesh();
		const bool bAlreadyTickedThisFrame = CharacterMesh->PoseTickedThisFrame();
	
		if (!bAlreadyTickedThisFrame)
		{
			TickCharacterPose(DeltaTime);
		}
	}
}

void UAuraCharacterMovementComponent::SmoothClientPosition(float DeltaSeconds)
{
	Super::SmoothClientPosition(DeltaSeconds);
    
	if (!CharacterOwner || CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
		return;

	FRotator CurrentRotation = CharacterOwner->GetActorRotation();
	FRotator TargetRotation = CharacterOwner->GetReplicatedMovement().Rotation;

	// 회전 차이가 거의 없으면 보간하지 않음
	if (CurrentRotation.Equals(TargetRotation, 0.1f))
		return;

	FRotator SmoothedRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaSeconds,
		15.f
	);

	CharacterOwner->SetActorRotation(SmoothedRotation);
}

float UAuraCharacterMovementComponent::GetMaxSpeed() const
{
	float ParentMaxSpeed = Super::GetMaxSpeed();
	
	if (auto AuraCharacter = Cast<AAuraCharacter>(GetCharacterOwner()))
	{
		if (auto AuraPS = AuraCharacter->GetPlayerState<AAuraPlayerState>())
		{
			if (UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AuraPS->GetAttributeSet()))
			{
				return AuraAS->GetMovementSpeed();
			}
		}
	}
	
	return ParentMaxSpeed;
}
