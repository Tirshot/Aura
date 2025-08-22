// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AbilityRangeIndicator.h"

#include "Character/AuraCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Player/AuraPlayerController.h"

AAbilityRangeIndicator::AAbilityRangeIndicator()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>("RootComponent");

	DecalComponent = CreateDefaultSubobject<UDecalComponent>("DecalComponent");
	DecalComponent->SetWorldRotation(FRotator(90.f, 0.f, 0.f));

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialAsset(TEXT("/Game/Blueprints/Actor/RangeIndicator/M_RangeIndicatorOutline.M_RangeIndicatorOutline"));
	if (MaterialAsset.Succeeded())
	{
		DecalComponent->SetMaterial(0, MaterialAsset.Object);
	}
	
	DecalComponent->SetupAttachment(RootComponent);

	CircleMaterial = CreateDefaultSubobject<UMaterialInstance>("CircleMaterial");
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> CircleMI(TEXT("/Game/Blueprints/Actor/RangeIndicator/MI_RangeIndicatorOutline_Inst.MI_RangeIndicatorOutline_Inst"));
	if (CircleMI.Succeeded())
	{
		CircleMaterial = CircleMI.Object;
	}

	RectMaterial = CreateDefaultSubobject<UMaterialInstance>("RectMaterial");
	static ConstructorHelpers::FObjectFinder<UMaterialInstance> RectMI(TEXT("/Game/Blueprints/Actor/RangeIndicator/MI_RangeIndicatorOutline_Rect.MI_RangeIndicatorOutline_Rect"));
	if (RectMI.Succeeded())
	{
		RectMaterial = RectMI.Object;
	}
}

void AAbilityRangeIndicator::BeginPlay()
{
	Super::BeginPlay();
	
	IndicatorInitialized.AddDynamic(this, &AAbilityRangeIndicator::ShowIndicator);
}

void AAbilityRangeIndicator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAbilityRangeIndicator::ShowIndicator(AActor* AvatarActor, bool bAttachToActor, ERangeShape Shape,
	const FVector& Location, float InRadius, float InWidth, float InHeight, float InAngle, const FVector& RGB)
{
	Radius = InRadius;
	Width = InWidth;
	Height = InHeight;
	ConeAngle = InAngle;
	RangeShape = Shape;

	FVector Color = RGB / 100.f;

	FHitResult HitResult;
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(AvatarActor);
	
	FVector HitStart = Location + FVector(0.f, 0.f, -300.f);
	FVector HitEnd = Location + FVector(0.f, 0.f, 300.f);
	FVector ImpactPoint = HitResult.ImpactPoint;
	
	GetWorld()->LineTraceSingleByChannel(HitResult, HitStart, HitEnd, ECC_Visibility, CollisionQueryParams);

	// TODO :: Shape에 따른 데칼 모양과 크기 결정하기
	switch (Shape)
	{
	case ERangeShape::ERS_Circle:	
		{
			if (IsValid(CircleMaterial))
			{
				DecalComponent->SetMaterial(0, CircleMaterial);
				DynamicMI = DecalComponent->CreateDynamicMaterialInstance();
			}
			
			// 액터에게 붙일 경우
			if (bAttachToActor)
			{
				AttachToActor(Owner, FAttachmentTransformRules::KeepRelativeTransform);
				FVector RelativeLocation = Owner->GetRootComponent()->GetComponentTransform().InverseTransformPosition(ImpactPoint);
		
				SetActorRelativeLocation(FVector(0.f, 0.f, RelativeLocation.Z));
			}
			else // 월드에 소환할 경우
			{
				SetActorLocation(FVector(Location.X, Location.Y, ImpactPoint.Z));
			}
			// 반지름 값에 따라 데칼 크기 수정
			float Scale = Radius / 200.f;
			SetActorScale3D(FVector(1.0f, Scale, Scale));
			break;
		}

	case ERangeShape::ERS_Rectangle:
		{
			if (IsValid(RectMaterial))
			{
				DecalComponent->SetMaterial(0, RectMaterial);
				DynamicMI = DecalComponent->CreateDynamicMaterialInstance();
			}
			
			// 액터에게 붙일 경우
			if (bAttachToActor)
			{
				AttachToActor(Owner, FAttachmentTransformRules::KeepRelativeTransform);
				FVector RelativeLocation = Owner->GetRootComponent()->GetComponentTransform().InverseTransformPosition(ImpactPoint);
		
				SetActorRelativeLocation(FVector(Height, 0.f, RelativeLocation.Z));
			}
			else // 월드에 소환할 경우
			{
				SetActorLocation(FVector(Location.X, Location.Y, ImpactPoint.Z));
			}
			SetActorScale3D(FVector(1.0f, Width / 200.f, Height / 200.f));
			break;
		}

	case ERangeShape::ERS_Cone:
		{

			break;
		}
	}
	// 다이내믹 머티리얼의 값 변경
	DynamicMI->SetVectorParameterValue("OutlineColor", Color);
}