

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
			// 슬로우 이펙트 적용
			if (auto* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
			{
				FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(SlowDownEffectClass, 1, OwnerASC->MakeEffectContext());
				
				const float MovementSpeed = UAuraAbilitySystemLibrary::GetAttributeValue(TargetActor, FAuraGameplayTags::Get().Attributes_Secondary_MovementSpeed);
				const float Magnitude = -MovementSpeed * SlowSpeedRatio;

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
		// 초기 이동속도 한 번만 저장
		if (!MovementSpeeds.Contains(AuraEnemy))
		{
			const FGameplayTag& MovementTag = FAuraGameplayTags::Get().Attributes_Secondary_MovementSpeed;
			const float MovementSpeed = UAuraAbilitySystemLibrary::GetAttributeValue(AuraEnemy, MovementTag);
				
			MovementSpeeds.Add(AuraEnemy, MovementSpeed);
		}
		
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
			if (auto* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(AuraEnemy))
			{
				FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(SlowDownDecayEffectClass, 1, OwnerASC->MakeEffectContext());

				if (float* ValPtr = MovementSpeeds.Find(AuraEnemy))
				{
					const float MovementSpeed = *ValPtr;
					const float Magnitude = MovementSpeed;

					// Set By Caller로 매그니튜드 지정 후 이펙트 적용
					SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.MovementSpeed"), Magnitude);
					OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
				}
			}
		}
		//
		MovementSpeeds.Remove(AuraEnemy);
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
			auto* OtherActor = OverlappedActors[i];
			if (!OtherActor || OtherActor->IsPendingKillPending())
			{
				OverlappedActors.RemoveAt(i);
				continue;
			}
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
