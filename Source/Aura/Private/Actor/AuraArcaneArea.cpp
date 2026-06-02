

#include "Actor/AuraArcaneArea.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Abilities/AuraArcaneAreaAbility.h"
#include "Character/AuraCharacterBase.h"
#include "Character/AuraEnemy.h"
#include "Components/DecalComponent.h"

AAuraArcaneArea::AAuraArcaneArea()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(Mesh);
	
	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	Sphere->SetupAttachment(RootComponent);

	Decal = CreateDefaultSubobject<UDecalComponent>("Decal");
	Decal->SetupAttachment(RootComponent);
}

void AAuraArcaneArea::BeginPlay()
{
	Super::BeginPlay();

	SetLifeSpan(LifeSpan);

	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AAuraArcaneArea::OnSphereOverlap);
	Sphere->OnComponentEndOverlap.AddDynamic(this, &AAuraArcaneArea::OnSphereEndOverlap);

	// 기본 메시 반경이 0.25m
	float ScaleFloat = SlowRadius / 50.f;
	Mesh->SetWorldScale3D(FVector(ScaleFloat, ScaleFloat, ScaleFloat));
}

void AAuraArcaneArea::Destroyed()
{
	OverlappedActors.Empty();

	if (ApplyEffectTimer.IsValid())
	{
		GetWorldTimerManager().ClearTimer(ApplyEffectTimer);
	}

	Super::Destroyed();
}

void AAuraArcaneArea::ApplySlowEffect()
{
	if (auto* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		for (auto* TargetActor : OverlappedActors)
		{
			// 아군에게 슬로우 적용 금지
			if (!IsValidOverlap(TargetActor))
				continue;
			
			// 슬로우 이펙트 적용
			if (auto* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
			{
				FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(SlowDownEffectClass, 1, OwnerASC->MakeEffectContext());
				const float Magnitude = -SlowSpeedRatio;

				// Set By Caller로 매그니튜드 지정 후 이펙트 적용
				SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.MovementSpeed"), Magnitude);
				OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}
}

void AAuraArcaneArea::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                      UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (AAuraEnemy* AuraEnemy = Cast<AAuraEnemy>(OtherActor))
	{
		DamageEffectParams.TargetAbilitySystemComponent = AuraEnemy->GetAbilitySystemComponent();

		OverlappedActors.AddUnique(OtherActor);
		
		// 슬로우 이펙트 타이머 설정
		FTimerDelegate EffectTimerDelegate;
		EffectTimerDelegate.BindUObject(this, &AAuraArcaneArea::ApplySlowEffect);

		// 오버랩이 시작하면 일정 시간마다 이펙트 중첩(스택 중첩)
		if (!ApplyEffectTimer.IsValid())
		{
			GetWorldTimerManager().SetTimer(
			   ApplyEffectTimer,
			   EffectTimerDelegate,
			   ApplyEffectPeriod,
			   true
			   );
		}

		// 데미지 이펙트 타이머 설정
		FTimerDelegate DamageTimerDelegate;
		DamageTimerDelegate.BindUObject(this, &AAuraArcaneArea::DamageAndKnockback);
		
		if (!ApplyDamageEffectTimer.IsValid())
		{
			GetWorldTimerManager().SetTimer(
			   ApplyDamageEffectTimer,
			   DamageTimerDelegate,
			   ApplyEffectPeriod,
			   true
			   );
		}
		
		if (bMindControl)
		{
			if (MindControlledUnitCount < 1)
			{
				// 마인드 컨트롤
				ChangeToActorTag(OtherActor, EActorTag::Player);
				MindControlledActor = OtherActor;
				MindControlledUnitCount++;
				
				// 마인드 컨트롤 타이머 설정
				FTimerDelegate MindControl;
				MindControl.BindUObject(this, &AAuraArcaneArea::ReleaseMindControl);
		
				if (!MindControlTimer.IsValid())
				{
					GetWorldTimerManager().SetTimer(
					   MindControlTimer,
					   MindControl,
					   MindControlDuration,
					   false
					   );
				}
			}
		}
	}
}

void AAuraArcaneArea::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (AAuraEnemy* AuraEnemy = Cast<AAuraEnemy>(OtherActor))
	{
		if (auto* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
		{
			// 이동속도 돌려주기
			auto Tag = FGameplayTag::RequestGameplayTag("Data.MovementSpeed.Slow");
			OwnerASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(Tag));
		}
		OverlappedActors.Remove(AuraEnemy);
	}
}

bool AAuraArcaneArea::IsValidOverlap(AActor* OtherActor)
{
	if (DamageEffectParams.SourceAbilitySystemComponent == nullptr)
		return false;

	AActor* SourceAvatarActor = DamageEffectParams.SourceAbilitySystemComponent->GetAvatarActor();

	// ���ΰ��� �浹 ����
	if (SourceAvatarActor == OtherActor)
		return false;

	// �Ʊ� ��� ����
	if (!UAuraAbilitySystemLibrary::IsNotFriend(SourceAvatarActor, OtherActor))
		return false;

	return true;
}

void AAuraArcaneArea::DamageAndKnockback()
{
	if (HasAuthority())
	{
		if (!bTakeDamage)
			return;

		// 유효성을 위해 거꾸로 순회
		for (int32 i = OverlappedActors.Num() - 1; i >= 0; --i)
		{
			if (!OverlappedActors.IsValidIndex(i))
				return;
			
			auto* OtherActor = OverlappedActors[i];
			if (!OtherActor || OtherActor->IsPendingKillPending())
			{
				OverlappedActors.RemoveAt(i);
				continue;
			}
			
			// 아군에게 데미지 적용 금지
			if (!IsValidOverlap(OtherActor))
				continue;
			
			if (auto* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor))
			{
				bool bIsKnockBack = true;
				
				// 벡터를 구하고 정규화
				FVector ToCenter = GetActorLocation() - OtherActor->GetActorLocation();
				ToCenter.Z = GetActorLocation().Z - Sphere->GetScaledSphereRadius();
				ToCenter.Normalize();
				
				// 보스는 넉백 무효
				if (OtherActor->ActorHasTag(FName("Boss")))
				{
					bIsKnockBack = false;
					DamageEffectParams.KnockbackChance = 0.f;
				}

				if (bIsKnockBack)
				{
					const FVector KnockbackDirection = ToCenter;
					const FVector KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;
					DamageEffectParams.KnockbackForce = KnockbackForce;
				}

				FVector DistanceVector = (OtherActor->GetActorLocation()-GetActorLocation());
				float Distance = DistanceVector.Length();
				
				if (Distance <= SlowRadius)
				{
					const FVector DeathImpulse = ToCenter * DamageEffectParams.DeathImpulseMagnitude;
					DamageEffectParams.DeathImpulse = DeathImpulse;
					DamageEffectParams.TargetAbilitySystemComponent = TargetASC;
				
					UAuraAbilitySystemLibrary::ApplyDamageEffect(DamageEffectParams);
				}
			}
		}
	}
}

void AAuraArcaneArea::ReleaseMindControl()
{
	ReturnToOrignalTag();
	MindControlledActor = nullptr;
}

bool AAuraArcaneArea::ChangeToActorTag(AActor* TargetActor, EActorTag ActorTag)
{
	if (!TargetActor)
		return false;
	
	if (TargetActor->ActorHasTag(FName("Boss")))
		return false;

	if (TargetActor->Tags.IsEmpty())
		return false;
	
	switch (ActorTag)
	{
	case Player:
		TargetActor->Tags.Remove(FName("Enemy"));
		TargetActor->Tags.AddUnique(FName("Player"));
		TargetActor->Tags.AddUnique(FName("MindControlled"));
		return true;

	case Enemy:
		TargetActor->Tags.Remove(FName("Player"));
		TargetActor->Tags.AddUnique(FName("Enemy"));
		TargetActor->Tags.AddUnique(FName("MindControlled"));
		return true;
	}
	
	return false;
}

void AAuraArcaneArea::ReturnToOrignalTag()
{
	if (!MindControlledActor)
		return;
	
	if (MindControlledActor->ActorHasTag(FName("Boss")))
		return;

	if (MindControlledActor->Tags.IsEmpty())
		return;
	
	if (MindControlledActor->ActorHasTag(FName("MindControlled")))
	{
		// 몬스터로 돌아오게하기
		if (MindControlledActor->ActorHasTag(FName("Player")))
		{
			MindControlledActor->Tags.Remove(FName("Player"));
			MindControlledActor->Tags.Remove(FName("MindControlled"));
			MindControlledActor->Tags.AddUnique(FName("Enemy"));
		}
		// 플레이어로 돌아오게하기
		else if (MindControlledActor->ActorHasTag(FName("Monster")))
		{
			MindControlledActor->Tags.Remove(FName("Monster"));
			MindControlledActor->Tags.Remove(FName("MindControlled"));
			MindControlledActor->Tags.AddUnique(FName("Player"));
		}
	}
}
