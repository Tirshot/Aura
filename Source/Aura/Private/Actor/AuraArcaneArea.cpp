

#include "Actor/AuraArcaneArea.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AuraGameplayTags.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Abilities/AuraArcaneAreaAbility.h"
#include "Character/AuraCharacter.h"
#include "Character/AuraCharacterBase.h"
#include "Character/AuraEnemy.h"
#include "Components/DecalComponent.h"
#include "GameFramework/PawnMovementComponent.h"

AAuraArcaneArea::AAuraArcaneArea()
{
	PrimaryActorTick.bCanEverTick = false;
	
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
}

void AAuraArcaneArea::Destroyed()
{
	// 어빌리티의 소환 함수내에서 바인딩 된 콜백 함수 호출됨
	OverlappedActors.Empty();

	if (ApplyEffectTimer.IsValid())
	{
		// 어트리뷰트를 감소시키는 이펙트 적용 해제
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
			if (auto* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor))
			{
				const FActiveGameplayEffect* ActiveGE = nullptr;
				FGameplayEffectQuery Query;
				Query.EffectDefinition = SlowDownEffectClass;

				TArray<FActiveGameplayEffectHandle> Handles = TargetASC->GetActiveEffects(Query);
				if (Handles.Num() > 0)
				{
					ActiveGE = TargetASC->GetActiveGameplayEffect(Handles[0]);
				}

				// 기존 이펙트에서 스택 가져오기
				int32 Stack = 1;
				if (ActiveGE)
				{
					Stack = ActiveGE->Spec.GetStackCount();
					if (Stack == 0)
						Stack = 1;
				}

				// 새로운 이펙트를 만들어 스택 덮어 씌우기
				FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(SlowDownEffectClass, 1, OwnerASC->MakeEffectContext());
				SpecHandle.Data->SetStackCount(Stack);

				float MovementSpeed = UAuraAbilitySystemLibrary::GetAttributeValue(TargetActor, FAuraGameplayTags::Get().Attributes_Secondary_MovementSpeed);
				float Magnitude = -MovementSpeed * 0.1f * Stack;
				SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.MovementSpeed"), Magnitude);

				// 기존 이펙트 제거 후 새로운 이펙트 다시 적용
				if (ActiveGE)
				{
					TargetASC->RemoveActiveGameplayEffect(Handles[0]);
					
					// // 스턴 스택 델리게이트 생성 및 바인딩
					// TargetASC->OnGameplayEffectStackChangeDelegate(Handles[0])->AddUObject(this, &AAuraArcaneArea::OnSlowStackChanged);
				}
				OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			}
		}
	}
}

void AAuraArcaneArea::OnSlowStackChanged(FActiveGameplayEffectHandle ActiveGEHandle, int32 NewStackCount,
	int32 OldStackCount)
{
	//감소할때만 체크
	if (OldStackCount > NewStackCount)
	{
		// 한 스택에 한 단계씩 다시 빨라지도록 이펙트 적용
		if (auto* TargetASC = ActiveGEHandle.GetOwningAbilitySystemComponent())
		{
			if (AActor* AvatarActor = TargetASC->GetAvatarActor())
			{
				float MovementSpeedBase = UAuraAbilitySystemLibrary::GetAttributeValue(AvatarActor, FAuraGameplayTags::Get().Attributes_Secondary_MovementSpeed, true);
				float Magnitude = 250.f;
	
				if (auto* OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
				{
					FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(SlowDownEffectClass, 1, OwnerASC->MakeEffectContext());
					SpecHandle.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag("Data.MovementSpeed"), Magnitude);
	
					OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
				}
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
		
		// 타이머 설정
		FTimerDelegate TimerDelegate;
		TimerDelegate.BindUFunction(this, FName("ApplySlowEffect"));

		// 오버랩이 시작하면 일정 시간마다 이펙트 중첩(스택 중첩)
		if (!ApplyEffectTimer.IsValid())
		{
			GetWorldTimerManager().SetTimer(
			   ApplyEffectTimer,
			   TimerDelegate,
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
		// const FActiveGameplayEffect* ActiveGE = nullptr;
		// FGameplayEffectQuery Query;
		// Query.EffectDefinition = SlowDownEffectClass;
		//
		// TArray<FActiveGameplayEffectHandle> Handles = AuraEnemy->GetAbilitySystemComponent()->GetActiveEffects(Query);
		// if (Handles.Num() > 0)
		// {
		// 	ActiveGE = AuraEnemy->GetAbilitySystemComponent()->GetActiveGameplayEffect(Handles[0]);
		// }
		// int32 DefaultWalkSpeed = UAuraAbilitySystemLibrary::GetAttributeValue(AuraEnemy, FAuraGameplayTags::Get().Attributes_Secondary_MovementSpeed, true);
		
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

void AAuraArcaneArea::DamageAndKnockback(AActor* OtherActor)
{
	if (HasAuthority())
	{
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
