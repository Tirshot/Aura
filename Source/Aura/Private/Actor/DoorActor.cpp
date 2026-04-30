// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/DoorActor.h"

#include "Net/UnrealNetwork.h"

ADoorActor::ADoorActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("DoorMesh");
	RootComponent = Mesh;
}

void ADoorActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 초기 회전값 저장
	InitialRotation = GetActorRotation();
	TargetRotation  = InitialRotation + FRotator(0.f, OpenAngle, 0.f);
	
	if (bIsOpen)
	{
		// 타임라인 없이 즉시 열린 상태로
		SetActorRotation(TargetRotation);
	}
	
	// 타임라인 설정
	if (DoorCurve)
	{
		FOnTimelineFloat TimelineTick;
		TimelineTick.BindUFunction(this, FName("OnDoorTimelineTick"));
		DoorTimeline.AddInterpFloat(DoorCurve, TimelineTick);
		DoorTimeline.SetLooping(false);
	}

	// 이미 열린 상태로 저장됐다면
	if (bIsOpen)
		OnRep_IsOpen();
}

void ADoorActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DoorTimeline.TickTimeline(DeltaTime);
}

void ADoorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ADoorActor, bIsOpen);
}

void ADoorActor::OpenDoor()
{
	// 서버에서만 실행
	if (!HasAuthority())
		return;
	
	if (bIsOpen)
		return;

	bIsOpen = true;
	ForceNetUpdate();
	OnRep_IsOpen();
}

void ADoorActor::OnRep_IsOpen()
{
	if (bIsOpen)
	{
		PlayOpenTimeline();
	}
}

void ADoorActor::OnDoorTimelineTick(float Value)
{
	FRotator NewRotation = FMath::Lerp(InitialRotation, TargetRotation, Value);
	SetActorRotation(NewRotation);
}

void ADoorActor::PlayOpenTimeline()
{
	if (DoorCurve)
	{
		DoorTimeline.PlayFromStart();
	}
	else
	{
		// 커브 없으면 즉시 회전
		SetActorRotation(TargetRotation);
	}
}

