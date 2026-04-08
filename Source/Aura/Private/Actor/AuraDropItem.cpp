

#include "Actor/AuraDropItem.h"

#include "Aura/Aura.h"
#include "Components/WidgetComponent.h"
#include "Game/AuraGameModeBase.h"
#include "Game/AuraGameStateBase.h"
#include "Net/UnrealNetwork.h"

AAuraDropItem::AAuraDropItem()
{
	bReplicates = true;
 	PrimaryActorTick.bCanEverTick = true;
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	SetRootComponent(Mesh);
	Mesh->SetCustomDepthStencilValue(CUSTOM_DEPTH_TAN);
	Mesh->MarkRenderStateDirty();
	
	ItemTitleWidget = CreateDefaultSubobject<UWidgetComponent>("ItemTitle");
	ItemTitleWidget->SetupAttachment(GetRootComponent());
	ItemTitleWidget->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	
	Sphere = CreateDefaultSubobject<USphereComponent>("Sphere");
	Sphere->SetupAttachment(Mesh);
	
	Sphere->OnComponentBeginOverlap.AddDynamic(this, &AAuraDropItem::OnSphereOverlap);
	Sphere->OnComponentEndOverlap.AddDynamic(this, &AAuraDropItem::OnSphereEndOverlap);
	
	MoveToComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MoveToComponent"));
	MoveToComponent->SetupAttachment(GetRootComponent());
}

void AAuraDropItem::BeginPlay()
{
	Super::BeginPlay();
	
	// 아이템 핸들이 지정되어 있지만, 아이템 데이터가 채워져있지 않으면 데이터 채움
	if (!ItemHandle.IsNull() && DropItemData.Name.IsNone())
	{
		if (auto FoundRow = ItemHandle.GetRow<FItemData>("FoundRow"))
		{
			DropItemData = *FoundRow;
			DropItemData.ItemCounts = ItemCount;
		}
	}
	
	InitializeItem(DropItemData);
	
	// 드랍된 아이템을 게임 스테이트에서 관리
	if (auto AuraState = GetWorld()->GetGameState<AAuraGameStateBase>())
	{
		AuraState->AddDroppedItem(this);
	}
}

void AAuraDropItem::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (auto AuraState = GetWorld()->GetGameState<AAuraGameStateBase>())
	{
		AuraState->RemoveDroppedItem(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void AAuraDropItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAuraDropItem::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AAuraDropItem, DropItemData);
}

void AAuraDropItem::HighlightActor_Implementation()
{
	Mesh->SetRenderCustomDepth(true);
}

void AAuraDropItem::UnHighlightActor_Implementation()
{
	Mesh->SetRenderCustomDepth(false);
}

void AAuraDropItem::SetMoveToLocation_Implementation(FVector& OutDestination)
{
	OutDestination = MoveToComponent->GetComponentLocation();
}

void AAuraDropItem::SetMeshAndMaterial()
{
	if (UStaticMesh* StaticMesh = DropItemData.StaticMesh)
	{
		Mesh->SetStaticMesh(StaticMesh);
	}

	if (UMaterialInterface* Mat = DropItemData.Material.Get())
	{
		Mesh->SetMaterial(0, Mat);
	}
}

void AAuraDropItem::InitializeItem(const FItemData& InItemData)
{
	DropItemData = InItemData;
	
	// 각 아이템에 고유 ID 부여 - 없을때만
	if (!DropItemData.UniqueID.IsValid())
		DropItemData.UniqueID = FGuid::NewGuid();
	
	UniqueID = DropItemData.UniqueID;
	
	SetMeshAndMaterial();
	
	OnDropItemInitialized.Broadcast(DropItemData.DisplayName);
}

void AAuraDropItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// UI 위젯 활성화
	ItemTitleWidget->SetVisibility(true);
}

void AAuraDropItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{

}

void AAuraDropItem::SetTitleWidgetVisibility(bool InValue)
{
	ItemTitleWidget->SetVisibility(InValue);
}

void AAuraDropItem::OnRep_DropItemData()
{
	SetMeshAndMaterial();
}