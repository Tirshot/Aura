// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/MagicCircle.h"
#include "Components/DecalComponent.h"
#include "Interaction/EnemyInterface.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"

AMagicCircle::AMagicCircle()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");

	MagicCircleDecal = CreateDefaultSubobject<UDecalComponent>("MagicCircleDecal");
	MagicCircleDecal->SetupAttachment(RootComponent);
}

void AMagicCircle::BeginPlay()
{
	Super::BeginPlay();
	
}

void AMagicCircle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}