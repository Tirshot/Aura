

#include "Actor/AuraDropItem.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Aura/Aura.h"
#include "Blueprint/UserWidget.h"
#include "Character/AuraCharacter.h"
#include "Components/WidgetComponent.h"
#include "Game/AuraGameModeBase.h"
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
	OnClickedDroppedItem.AddDynamic(this, &AAuraDropItem::Server_AddItemToCharacter);
	
	MoveToComponent = CreateDefaultSubobject<USceneComponent>(TEXT("MoveToComponent"));
	MoveToComponent->SetupAttachment(GetRootComponent());
}

void AAuraDropItem::BeginPlay()
{
	Super::BeginPlay();
	
	Mesh->OnClicked.AddDynamic(this, &AAuraDropItem::OnClickedItem);
	ItemTitleWidget->OnClicked.AddDynamic(this, &AAuraDropItem::OnClickedItem);
	
	// 아이템 핸들이 지정되어 있지만, 아이템 데이터가 채워져있지 않으면 데이터 채움
	if (!ItemHandle.IsNull() && DropItemData.Name.IsNone())
	{
		if (auto FoundRow = ItemHandle.GetRow<FItemData>("FoundRow"))
		{
			DropItemData = *FoundRow;
			DropItemData.ItemCounts = ItemCount;
			InitializeItem(DropItemData);
		}
	}
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

void AAuraDropItem::InitializeItem(const FItemData& InItemData)
{
	DropItemData = InItemData;
	OnRep_ItemData();
	
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

void AAuraDropItem::OnClickedItem(UPrimitiveComponent* TouchedComponent, FKey ButtonPressed)
{
	if (!GetOwner())
		return;
	
	if (ButtonPressed == EKeys::LeftMouseButton)
	{
		// 거리 계산
		FVector ItemLocation = GetActorLocation();
		FVector OwnerLocation = GetOwner()->GetActorLocation();
		
		// 제곱근 계산을 하지 않으면 더 빠름
		float DistSquare = FVector::DistSquared(ItemLocation, OwnerLocation);
		
		// 약 160cm
		if (DistSquare < 25000)
		{
			Server_AddItemToCharacter(GetOwner());
		}
	}
}

void AAuraDropItem::Server_AddItemToCharacter_Implementation(AActor* ItemOwner)
{
	if (AAuraGameModeBase* AuraGM = GetWorld()->GetAuthGameMode<AAuraGameModeBase>())
	{
		if (AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(ItemOwner))
		{
			if (AuraGM->GiveItemToCharacter(AuraCharacter, DropItemData.Name, ItemCount))
			{
				Destroy();
			}
			else
			{
				// 슬롯 부족, 또는 아이템 데이터 검색 실패
				UAuraAbilitySystemLibrary::ApplyMessageTagEffectToSelf(FGameplayTag::RequestGameplayTag("Message.InventoryFull"), AuraCharacter, FText());
			}
		}
	}
}

void AAuraDropItem::OnRep_ItemData()
{
	if (UStaticMesh* StaticMesh = DropItemData.StaticMesh)
	{
		Mesh->SetStaticMesh(StaticMesh);
	}

	if (UMaterialInterface* Mat = DropItemData.Material)
	{
		Mesh->SetMaterial(0, Mat);
	}
}

