// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/GhostEffectActor.h"

AGhostEffectActor::AGhostEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("GhostMesh"));
	SetRootComponent(Mesh);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGhostEffectActor::SetGhostActorMesh(USkeletalMeshComponent* SourceMesh, UMaterialInterface* GhostMaterial)
{
	Mesh->SetSkeletalMeshAsset(SourceMesh->GetSkeletalMeshAsset());

	// 포즈 정지
	Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	
	// 머티리얼 변경
	for (int i = 0; i < Mesh->GetNumMaterials(); i++)
	{
		Mesh->SetMaterial(i, GhostMaterial);
	}
}

