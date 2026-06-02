// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/InstantKillActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Interaction/CombatInterface.h"

AInstantKillActor::AInstantKillActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	KillBox = CreateDefaultSubobject<UBoxComponent>(TEXT("KillBox"));
	SetRootComponent(KillBox);

	KillBox->SetCollisionObjectType(ECC_WorldStatic);
	KillBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	KillBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	KillBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AInstantKillActor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		KillBox->OnComponentBeginOverlap.AddDynamic(this, &AInstantKillActor::OnKillBoxOverlap);
	}
}

void AInstantKillActor::OnKillBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor) return;

	if (OtherActor->Implements<UCombatInterface>())
	{
		if (ICombatInterface::Execute_IsDead(OtherActor))
		{
			return;
		}
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
	if (TargetASC)
	{
		// Aura 프로젝트의 사망 태그가 있다면 무시
		if (TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag("State.Dead")))
		{
			return;
		}
	}

	if (OtherActor->Implements<UCombatInterface>())
	{
		FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
		Context.AddSourceObject(this);
		auto Spec = TargetASC->MakeOutgoingSpec(InstantDeathGEClass, 1.f, Context);
		TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	}
}
