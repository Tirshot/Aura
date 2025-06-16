// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GhostEffectActor.generated.h"

UCLASS()
class AURA_API AGhostEffectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AGhostEffectActor();

	void SetGhostActorMesh(USkeletalMeshComponent* SourceMesh, UMaterialInterface* GhostMaterial);

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USkeletalMeshComponent* Mesh;
};
