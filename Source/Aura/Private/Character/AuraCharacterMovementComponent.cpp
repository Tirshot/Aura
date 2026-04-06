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
	bServerAcceptClientAuthoritativePosition = true;
	bNetworkAlwaysReplicateTransformUpdateTimestamp = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UAuraCharacterMovementComponent::ServerAutonomousProxyTick(float DeltaTime)
{
	Super::ServerAutonomousProxyTick(DeltaTime);
	
	// //매 프레임 틱마다 캐릭터 포즈 틱을 실행하게 함
	// if (IsNetMode(NM_ListenServer) && CharacterOwner && CharacterOwner->GetMesh())
	// {	
	// 	USkeletalMeshComponent* CharacterMesh = CharacterOwner->GetMesh();
	// 	const bool bAlreadyTickedThisFrame = CharacterMesh->PoseTickedThisFrame();
	//
	// 	if (!bAlreadyTickedThisFrame)
	// 	{
	// 		TickCharacterPose(DeltaTime);
	// 	}
	// }
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
