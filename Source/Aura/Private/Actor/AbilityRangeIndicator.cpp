// Fill out your copyright notice in the Description page of Project Settings.


#include "Actor/AbilityRangeIndicator.h"

#include "Character/AuraEnemy.h"
#include "Components/DecalComponent.h"
#include "Net/UnrealNetwork.h"

AAbilityRangeIndicator::AAbilityRangeIndicator()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bReplicates = true;

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
	
	// 데칼 오프셋 설정
	DecalComponent->SetRelativeLocation(FVector(1.f, 0.f, 0.f));
}

void AAbilityRangeIndicator::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AAbilityRangeIndicator, IndicatorColor);
	DOREPLIFETIME(AAbilityRangeIndicator, bRotate);
	DOREPLIFETIME(AAbilityRangeIndicator, RotationSpeed);
	DOREPLIFETIME(AAbilityRangeIndicator, RangeParams);
}

void AAbilityRangeIndicator::BeginPlay()
{
	Super::BeginPlay();
	
	DynamicMI = DecalComponent->CreateDynamicMaterialInstance();
}

void AAbilityRangeIndicator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// 회전 기믹에 따라 데칼도 회전시킴
	if (bRotate)
	{
		AddActorLocalRotation(FRotator(0.f, RotationSpeed * DeltaTime, 0.f));
	}
}

void AAbilityRangeIndicator::ShowIndicatorOwnerOnly()
{
	// 플레이어가 소환한 데칼인지 체크
	if (APawn* AvatarPawn = Cast<APawn>(GetOwner()))
	{
		if (AvatarPawn->IsPlayerControlled())
		{
			// 컨트롤 중인 플레이어와 데칼을 소환한 플레이어가 동일한지 판단
			bool bCheck = AvatarPawn->IsLocallyControlled();
			if (!bCheck)
			{
				// 아니라면 데칼 숨김
				DecalComponent->SetHiddenInGame(true);
			}
			else
			{
				// 맞다면 데칼 보여짐
				DecalComponent->SetHiddenInGame(false);
			}
		}
		else
		{
			// 몬스터가 소환한 데칼이라면 보여줌
			DecalComponent->SetHiddenInGame(false);
		}
	}
}

void AAbilityRangeIndicator::InitializeIndicatorParams(AActor* InAvatarActor, bool bInAttachToActor,
	ERangeShape InShape, const FVector& InStartLocation, float InRadius, float InWidth, float InHeight, float InAngle,
	const FVector& InRGB)
{
	// 서버에서 데이터 넣기
	SetOwner(InAvatarActor);
	
	RangeParams.bAttachToActor = bInAttachToActor;
	RangeParams.RangeShape = InShape;
	RangeParams.StartLocation = InStartLocation;
	RangeParams.Radius = InRadius;
	RangeParams.Width = InWidth;
	RangeParams.Height = InHeight;
	RangeParams.ConeAngle = InAngle;
	RangeParams.IndicatorColor = InRGB / 100.f;

	// 멤버 변수들 대입
	IndicatorColor = RangeParams.IndicatorColor;

	// 2. 호스트(리슨 서버 플레이어)를 위해 즉시 비주얼 생성
	InitIndicatorVisual();
}

// void AAbilityRangeIndicator::ShowIndicator(AActor* AvatarActor, bool bAttachToActor, ERangeShape Shape,
//                                            const FVector& StartLocation, float InRadius, float InWidth, float InHeight, float InAngle, const FVector& RGB)
// {
// 	FVector Color = RGB / 100.f;
//
// 	// 다이내믹 머티리얼의 값 변경
// 	IndicatorColor = Color;
// 	
// 	// 데칼 모양 레플리케이션, 다이내믹 머티리얼 인스턴스 생성 후 색 변경
// 	// 몬스터를 제외하고, 소유자에게만 데칼 보임
// 	UpdateDecalVisual();
// 	
// 	FHitResult HitResult;
// 	FCollisionQueryParams CollisionQueryParams;
// 	CollisionQueryParams.AddIgnoredActor(AvatarActor);
// 	
// 	FVector HitStart = StartLocation + FVector(0.f, 0.f, -300.f);
// 	FVector HitEnd = StartLocation + FVector(0.f, 0.f, 300.f);
// 	
// 	if (bRotate)
// 	{
// 		RotateStartLocation = StartLocation;
// 	}
// 	
// 	GetWorld()->LineTraceSingleByChannel(HitResult, HitStart, HitEnd, ECC_Visibility, CollisionQueryParams);
// 	FVector ImpactPoint = HitResult.ImpactPoint;
//
// 	// Shape에 따른 데칼 모양과 크기 결정
// 	switch (Shape)
// 	{
// 	case ERangeShape::ERS_Circle:	
// 		{
// 			if (IsValid(CircleMaterial))
// 			{
// 				DecalComponent->SetMaterial(0, CircleMaterial);
// 				DynamicMI->SetVectorParameterValue("OutlineColor", IndicatorColor);
// 				OnRep_IndicatorColor();
// 			}
// 			
// 			// 액터에게 붙일 경우
// 			if (bAttachToActor)
// 			{
// 				AttachToComponent(AvatarActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
// 				FVector RelativeLocation = AvatarActor->GetRootComponent()->GetComponentTransform().InverseTransformPosition(ImpactPoint);
// 		
// 				SetActorRelativeLocation(FVector(0.f, 0.f, RelativeLocation.Z));
// 			}
// 			else // 월드에 소환할 경우
// 			{
// 				SetActorLocation(FVector(StartLocation.X, StartLocation.Y, ImpactPoint.Z));
// 			}
// 			// 반지름 값에 따라 데칼 크기 수정
// 			float Scale = Radius / 200.f;
// 			SetActorScale3D(FVector(1.0f, Scale, Scale));
// 			break;
// 		}
//
// 	case ERangeShape::ERS_Rectangle:
// 		{
// 			if (IsValid(RectMaterial))
// 			{
// 				DecalComponent->SetMaterial(0, RectMaterial);
// 				DynamicMI->SetVectorParameterValue("OutlineColor", IndicatorColor);
// 				OnRep_IndicatorColor();
// 			}
// 			
// 			// 액터에게 붙일 경우
// 			if (bAttachToActor)
// 			{
// 				AttachToComponent(AvatarActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
// 				FVector RelativeLocation = AvatarActor->GetRootComponent()->GetComponentTransform().InverseTransformPosition(ImpactPoint);
// 				
// 				SetActorRelativeLocation(FVector(0.f, 0.f, RelativeLocation.Z));
// 				
// 				// 몬스터의 데칼만 옮김
// 				if (Cast<AAuraEnemy>(AvatarActor))
// 					DecalComponent->SetRelativeLocation(FVector(Height, 0.f, 0.f));
// 			}
// 			else // 월드에 소환할 경우
// 			{
// 				SetActorLocation(FVector(StartLocation.X, StartLocation.Y, ImpactPoint.Z));
// 			}
// 			SetActorScale3D(FVector(1.0f, Width / DecalComponent->DecalSize.Y, Height / DecalComponent->DecalSize.Z));
// 			break;
// 		}
// 		
// 	case ERangeShape::ERS_RectangleAndCircle:
// 		{
// 			if (IsValid(RectMaterial))
// 			{
// 				DecalComponent->SetMaterial(0, RectMaterial);
// 				OnRep_IndicatorColor();
// 				DynamicMI->SetVectorParameterValue("OutlineColor", IndicatorColor);
// 			}
// 			
// 			// 액터에게 붙일 경우
// 			if (bAttachToActor)
// 			{
// 				AttachToComponent(AvatarActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
// 				FVector RelativeLocation = AvatarActor->GetRootComponent()->GetComponentTransform().InverseTransformPosition(ImpactPoint);
// 		
// 				SetActorRelativeLocation(FVector(Height, 0.f, RelativeLocation.Z));
// 			}
// 			else // 월드에 소환할 경우
// 			{
// 				SetActorLocation(FVector(StartLocation.X, StartLocation.Y, ImpactPoint.Z));
// 			}
// 			SetActorScale3D(FVector(1.0f, Width / 200.f, Height / 200.f));
// 			break;
// 		}
//
// 	case ERangeShape::ERS_Cone:
// 		{
//
// 			break;
// 		}
// 	}
// }

void AAbilityRangeIndicator::UpdateDecalVisual()
{
	UMaterialInterface* Target = nullptr;
	switch (RangeParams.RangeShape)
	{
	case ERangeShape::ERS_Circle:
		Target = CircleMaterial;
		break;
	case ERangeShape::ERS_Rectangle:
		Target = RectMaterial;
		break;
	case ERangeShape::ERS_RectangleAndCircle:
		Target = RectMaterial;
		break;
	}

	if (Target)
	{
		DecalComponent->SetMaterial(0, Target);
	}

	DynamicMI = DecalComponent->CreateDynamicMaterialInstance();
	if (DynamicMI)
	{
		DynamicMI->SetVectorParameterValue("OutlineColor", IndicatorColor);
	}
	
	ShowIndicatorOwnerOnly();
}

void AAbilityRangeIndicator::OnRep_RangeParams()
{
	InitIndicatorVisual();
}

void AAbilityRangeIndicator::InitIndicatorVisual()
{
	UpdateDecalVisual();
	
	if (!IsValid(GetOwner()))
		return;

    FHitResult HitResult;
    FCollisionQueryParams CollisionQueryParams;
    CollisionQueryParams.AddIgnoredActor(GetOwner());
    
    FVector HitStart = RangeParams.StartLocation + FVector(0.f, 0.f, -300.f);
    FVector HitEnd = RangeParams.StartLocation + FVector(0.f, 0.f, 300.f);
    
    if (bRotate)
    {
       RotateStartLocation = RangeParams.StartLocation;
    }
    
    GetWorld()->LineTraceSingleByChannel(HitResult, HitStart, HitEnd, ECC_Visibility, CollisionQueryParams);
    FVector ImpactPoint = HitResult.ImpactPoint;

    // Shape에 따른 데칼 모양과 크기 결정
    switch (RangeParams.RangeShape)
    {
    case ERangeShape::ERS_Circle:  
       {
          if (IsValid(CircleMaterial))
          {
             DecalComponent->SetMaterial(0, CircleMaterial);
             if (DynamicMI) DynamicMI->SetVectorParameterValue("OutlineColor", IndicatorColor);
             OnRep_IndicatorColor();
          }
          
          if (RangeParams.bAttachToActor)
          {
             AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
             FVector RelativeLocation = GetOwner()->GetRootComponent()->GetComponentTransform().InverseTransformPosition(ImpactPoint);
             SetActorRelativeLocation(FVector(0.f, 0.f, RelativeLocation.Z));
          }
          else
          {
             SetActorLocation(FVector(RangeParams.StartLocation.X, RangeParams.StartLocation.Y, ImpactPoint.Z));
          }
          
          float Scale = RangeParams.Radius / DecalComponent->DecalSize.Y;
          SetActorScale3D(FVector(1.0f, Scale, Scale));
          break;
       }

    case ERangeShape::ERS_Rectangle:
       {
          if (IsValid(RectMaterial))
          {
             DecalComponent->SetMaterial(0, RectMaterial);
             if (DynamicMI) DynamicMI->SetVectorParameterValue("OutlineColor", IndicatorColor);
             OnRep_IndicatorColor();
          }
          
          if (RangeParams.bAttachToActor)
          {
             AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
             FVector RelativeLocation = GetOwner()->GetRootComponent()->GetComponentTransform().InverseTransformPosition(ImpactPoint);
             SetActorRelativeLocation(FVector(0.f, 0.f, RelativeLocation.Z));
             
             if (Cast<AAuraEnemy>(GetOwner()))
                DecalComponent->SetRelativeLocation(FVector(RangeParams.Height, 0.f, 0.f));
          }
          else
          {
             SetActorLocation(FVector(RangeParams.StartLocation.X, RangeParams.StartLocation.Y, ImpactPoint.Z));
          }
          
          SetActorScale3D(FVector(1.0f, RangeParams.Width / DecalComponent->DecalSize.Y, RangeParams.Height / DecalComponent->DecalSize.Z));
          break;
       }
       
    case ERangeShape::ERS_RectangleAndCircle:
       {
          if (IsValid(RectMaterial))
          {
             DecalComponent->SetMaterial(0, RectMaterial);
             OnRep_IndicatorColor();
             if (DynamicMI) DynamicMI->SetVectorParameterValue("OutlineColor", IndicatorColor);
          }
          
          if (RangeParams.bAttachToActor)
          {
             AttachToComponent(GetOwner()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
             FVector RelativeLocation = GetOwner()->GetRootComponent()->GetComponentTransform().InverseTransformPosition(ImpactPoint);
             SetActorRelativeLocation(FVector(RangeParams.Height, 0.f, RelativeLocation.Z));
          }
          else
          {
             SetActorLocation(FVector(RangeParams.StartLocation.X, RangeParams.StartLocation.Y, ImpactPoint.Z));
          }
          SetActorScale3D(FVector(1.0f, RangeParams.Width / DecalComponent->DecalSize.Y, RangeParams.Height / DecalComponent->DecalSize.Z));
          break;
       }

    case ERangeShape::ERS_Cone:
       {
          break;
       }
    }
}

void AAbilityRangeIndicator::OnRep_IndicatorColor()
{
	UpdateDecalVisual();
}

void AAbilityRangeIndicator::OnRep_Owner()
{
	Super::OnRep_Owner();
	
	UpdateDecalVisual();
}
