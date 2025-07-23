// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AbilityRangeIndicator.h"

#include "Components/DecalComponent.h"

AAbilityRangeIndicator::AAbilityRangeIndicator()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");

	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetupAttachment(RootComponent);
}

void AAbilityRangeIndicator::BeginPlay()
{
	Super::BeginPlay();
	
}

void AAbilityRangeIndicator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAbilityRangeIndicator::ShowIndicator(ERangeShape Shape, const FVector& Location, float InRadius, float InWidth, float InHeight, float InAngle)
{
	
}

