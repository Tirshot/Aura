// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/AuraHUD.h"

#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Player/AuraPlayerController.h"
#include "Player/AuraPlayerState.h"
#include "UI/ViewModel/MVVM_AbilityCard.h"
#include "UI/ViewModel/MVVM_CardSelection.h"
#include "UI/ViewModel/MVVM_DebugMenu.h"
#include "UI/Widget/AuraCenterDescriptionWidget.h"
#include "UI/Widget/AuraMessageBoxWidget.h"
#include "UI/Widget/AuraOverlayWidget.h"
#include "UI/Widget/AuraUserWidget.h"
#include "UI/Widget/LoadScreenWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "UI/WidgetController/SpellMenuWidgetController.h"
#include "UI/WidgetController/GameOverWidgetController.h"
#include "UI/WidgetController/ItemToolTipWidgetController.h"
#include "UI/WidgetController/SaveProgressWidgetController.h"
#include "UI/WidgetController/SettingsMenuWidgetController.h"
#include "UI/WidgetController/SpellUpgradesWidgetController.h"

UOverlayWidgetController *AAuraHUD::GetOverlayWidgetController(const FWidgetControllerParams &WCParams)
{
    if (OverlayWidgetController == nullptr)
    {   // 없으면 생성
        OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
        OverlayWidgetController->SetWidgetControllerParams(WCParams);
        OverlayWidgetController->BindCallbacksToDependencies();
    }

    return OverlayWidgetController;
}

UAttributeMenuWidgetController* AAuraHUD::GetAttributeMenuWidgetController(const FWidgetControllerParams& WCParams)
{
    if (AttributeMenuWidgetController == nullptr)
    {   // 없으면 생성
        AttributeMenuWidgetController = NewObject<UAttributeMenuWidgetController>(this, AttributeMenuWidgetControllerClass);
        AttributeMenuWidgetController->SetWidgetControllerParams(WCParams);
        AttributeMenuWidgetController->BindCallbacksToDependencies();
    }

    return AttributeMenuWidgetController;
}

USpellMenuWidgetController* AAuraHUD::GetSpellMenuWidgetController(const FWidgetControllerParams& WCParams)
{
    if (SpellMenuWidgetController == nullptr)
    {   // 없으면 생성
        SpellMenuWidgetController = NewObject<USpellMenuWidgetController>(this, SpellMenuWidgetControllerClass);
        SpellMenuWidgetController->SetWidgetControllerParams(WCParams);
        SpellMenuWidgetController->BindCallbacksToDependencies();
    }

    return SpellMenuWidgetController;
}

USpellUpgradesWidgetController* AAuraHUD::GetSpellUpgradesWidgetController(const FWidgetControllerParams& WCParams)
{
    if (SpellUpgradesWidgetController == nullptr)
    {   // 없으면 생성
        SpellUpgradesWidgetController = NewObject<USpellUpgradesWidgetController>(this, SpellUpgradesWidgetControllerClass);
        SpellUpgradesWidgetController->SetWidgetControllerParams(WCParams);
        SpellUpgradesWidgetController->BindCallbacksToDependencies();
    }

    return SpellUpgradesWidgetController;
}

UGameOverWidgetController* AAuraHUD::GetGameOverWidgetController(const FWidgetControllerParams& WCParams)
{
    if (GameOverWidgetController == nullptr)
    {   // 없으면 생성
        GameOverWidgetController = NewObject<UGameOverWidgetController>(this, GameOverWidgetControllerClass);
        GameOverWidgetController->SetWidgetControllerParams(WCParams);
        GameOverWidgetController->BroadcastInitialValues();
        GameOverWidgetController->BindCallbacksToDependencies();
    }
    return GameOverWidgetController;
}

USaveProgressWidgetController* AAuraHUD::GetSaveProgressWidgetController(const FWidgetControllerParams& WCParams)
{
    if (SaveProgressWidgetController == nullptr)
    {   // 없으면 생성
        SaveProgressWidgetController = NewObject<USaveProgressWidgetController>(this, SaveProgressWidgetControllerClass);
        SaveProgressWidgetController->SetWidgetControllerParams(WCParams);
        SaveProgressWidgetController->BindCallbacksToDependencies();
    }

    return SaveProgressWidgetController;
}

USettingsMenuWidgetController* AAuraHUD::GetSettingsMenuWidgetController(const FWidgetControllerParams& WCParams)
{
    if (SettingsMenuWidgetController == nullptr)
    {   // 없으면 생성
        SettingsMenuWidgetController = NewObject<USettingsMenuWidgetController>(this, SettingsMenuWidgetControllerClass);
        SettingsMenuWidgetController->SetWidgetControllerParams(WCParams);
        SettingsMenuWidgetController->BindCallbacksToDependencies();
    }

    return SettingsMenuWidgetController;
}

UItemToolTipWidgetController* AAuraHUD::GetItemToolTipWidgetController(const FWidgetControllerParams& WCParams)
{
    if (ItemToolTipWidgetController == nullptr)
    {   // 없으면 생성
        ItemToolTipWidgetController = NewObject<UItemToolTipWidgetController>(this, ItemToolTipWidgetControllerClass);
        ItemToolTipWidgetController->SetWidgetControllerParams(WCParams);
        ItemToolTipWidgetController->BindCallbacksToDependencies();
    }

    return ItemToolTipWidgetController;
}

UMVVM_CardSelection* AAuraHUD::GetCardSelectionViewModel()
{
    if (CardSelectionViewModel == nullptr)
    {   // 없으면 생성
        CardSelectionViewModel = NewObject<UMVVM_CardSelection>(this, CardSelectionViewModelClass);
        CardSelectionViewModel->InitializeSlot();
        CardSelectionViewModel->OnCardSelectionViewModelInitialized.Broadcast();
    }

    return CardSelectionViewModel;
}

UMVVM_DebugMenu* AAuraHUD::GetDebugMenuViewModel(const FWidgetControllerParams& WCParams)
{
    if (DebugMenuViewModel == nullptr)
    {   // 없으면 생성
        DebugMenuViewModel = NewObject<UMVVM_DebugMenu>(this, DebugMenuViewModelClass);
        DebugMenuViewModel->ViewModelInitialized();
    }

    return DebugMenuViewModel;
}

UMVVM_Inventory* AAuraHUD::GetInventoryViewModel(const FWidgetControllerParams& WCParams)
{
    if (!InventoryMenuViewModel)
    {
        InventoryMenuViewModel = NewObject<UMVVM_Inventory>(this, InventoryMenuViewModelClass);
        InventoryMenuViewModel->BindDependencies(WCParams);
    }
    return InventoryMenuViewModel;
}

void AAuraHUD::InitOverlay(APlayerController *PC, APlayerState *PS, UAbilitySystemComponent *ASC, UAttributeSet *AS)
{
    // 위젯과 위젯 컨트롤러 생성
    checkf(OverlayWidgetClass, TEXT("Overlay Widget not Initialized. BP_AuraHUD"));
    checkf(OverlayWidgetControllerClass, TEXT("Overlay Widget Controller not Initialized. BP_AuraHUD"));

    // 위젯 생성 후 오라 유저 위젯으로 캐스팅
    if (!IsValid(OverlayWidget))
    {
        UUserWidget* Widget = CreateWidget<UUserWidget>(PC, OverlayWidgetClass);
        OverlayWidget = Cast<UAuraOverlayWidget>(Widget);
    }
    
    // 구조체에 할당 후 오버레이 위젯 컨트롤러를 초기화
    const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
    UOverlayWidgetController* NewOverlayWidgetController = GetOverlayWidgetController(WidgetControllerParams);

    // 위젯에 위젯 컨트롤러를 연결
    OverlayWidget->SetWidgetController(NewOverlayWidgetController);
    NewOverlayWidgetController->OverlayWidget = OverlayWidget;
    NewOverlayWidgetController->ShowOverlayWidget(true);
    
    // 유효한 속성 세트와 위젯 컨트롤러를 가짐 -> 값 초기화 가능
    NewOverlayWidgetController->BroadcastInitialValues();
    if (!OverlayWidget->IsInViewport())
    {
        OverlayWidget->AddToViewport();
        if (UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(ASC))
        {
           AuraASC->AbilitiesGivenDelegate.Broadcast();
        }
        NewOverlayWidgetController->SetXPBarPercentToOwnValue();
    }
    
    // 게임오버 위젯 컨트롤러 생성
    GameOverWidgetController = GetGameOverWidgetController(WidgetControllerParams);
    GameOverWidgetController->BindCallbacksToDependencies();

    // 저장중 위젯 컨트롤러 생성 및 연결
    SaveProgressWidgetController = GetSaveProgressWidgetController(WidgetControllerParams);

    // 스펠 메뉴 위젯 컨트롤러 생성
    SpellMenuWidgetController = GetSpellMenuWidgetController(WidgetControllerParams);
    
    // 스펠 업그레이드 위젯 컨트롤러 생성
    SpellUpgradesWidgetController = GetSpellUpgradesWidgetController(WidgetControllerParams);

    // 설정 메뉴 위젯 컨트롤러 생성
    SettingsMenuWidgetController = GetSettingsMenuWidgetController(WidgetControllerParams);
    
    // 카드 선택 UI 뷰 모델 생성
    CardSelectionViewModel = GetCardSelectionViewModel();
    
    // 툴팁 위젯 컨트롤러 생성
    ToolTipViewModel = GetItemToolTipWidgetController(WidgetControllerParams);

    if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(PS))
    {
        if (!AuraPS->OnUpgradeCardsInitializedDelegate.IsAlreadyBound(this, &AAuraHUD::HandleRandomAbilityUpgradeInfos))
            AuraPS->OnUpgradeCardsInitializedDelegate.AddDynamic(this, &AAuraHUD::HandleRandomAbilityUpgradeInfos);
    }

    // 디버그 메뉴 뷰 모델 생성
    if (HasAuthority())
    {
        DebugMenuViewModel = NewObject<UMVVM_DebugMenu>(this, DebugMenuViewModelClass);
        // DebugMenuViewModel->ViewModelInitialized();
    }

    InventoryMenuViewModel = GetInventoryViewModel(WidgetControllerParams);
}

void AAuraHUD::ResetWidgetControllerAndViewModels()
{
    // SaveProgressWidgetController = nullptr;
    // OverlayWidgetController = nullptr;
    // AttributeMenuWidgetController = nullptr;
    // SpellMenuWidgetController = nullptr;
    // GameOverWidgetController = nullptr;
    // SettingsMenuWidgetController = nullptr;
}

void AAuraHUD::BeginPlay()
{
    Super::BeginPlay();
    
    if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetOwningPlayerController()))
    {
        AuraPC->OnReviveTimerEnd.AddDynamic(this, &AAuraHUD::ResetWidgetControllerAndViewModels);
    }
}

void AAuraHUD::OnPlayerStateCardsOninitialized(TArray<FAuraAbilityUpgradeInfo>& UpgradeInfos)
{
    HandleRandomAbilityUpgradeInfos(UpgradeInfos);
}

void AAuraHUD::CreateSaveProgressWidget()
{
    // 위젯 생성
    if (!SaveProgressWidget || !SaveProgressWidgetClass)
    {
        SaveProgressWidget = CreateWidget<UAuraUserWidget>(GetWorld(), SaveProgressWidgetClass);
        SaveProgressWidget->SetWidgetController(SaveProgressWidgetController);
    }

    // 뷰포트에 없으면 추가
    if (SaveProgressWidget && SaveProgressWidget->IsInViewport() == false)
    {
        SaveProgressWidget->AddToViewport();
    }
}

void AAuraHUD::RemoveSaveProgressWidget()
{
    if (SaveProgressWidget && SaveProgressWidget->IsInViewport())
    {
        // 델리게이트 호출
        SaveProgressWidgetController->RemoveWidget();
    }
}

void AAuraHUD::CreateMessageWidget(TSubclassOf<UAuraUserWidget> MessageWidgetClass, FText Message, UTexture2D* Icon)
{
    UAuraUserWidget* MessageWidget = CreateWidget<UAuraUserWidget>(GetOwningPlayerController(), MessageWidgetClass);
    if (UAuraMessageBoxWidget* AuraMessageBox = Cast<UAuraMessageBoxWidget>(MessageWidget))
    {
        // 메시지 박스에 내용 추가
        OverlayWidget->WBP_MessageBox->AddTextMessageToBox(Message);
        return;
    }
					
    // 중앙 설명 텍스트
    if (UAuraCenterDescriptionWidget* CenterWidget = Cast<UAuraCenterDescriptionWidget>(MessageWidget))
    {
        OverlayWidget->WBP_CenterTutorialDescription->TextBlock->SetText(Message);
        return;
    }
			
    // 팝업 텍스트
    if (MessageWidget->Implements<UMessageInterface>())
    {
        IMessageInterface::Execute_SetMessage(MessageWidget, Message, Icon);
        MessageWidget->AddToViewport();
    }
}

void AAuraHUD::HandleRandomAbilityUpgradeInfos(TArray<FAuraAbilityUpgradeInfo>& UpgradeInfos)
{
    // 뷰 생성
    if (!IsValid(CardSelectionWidget))
    {
        CardSelectionWidget = CreateWidget<ULoadScreenWidget>(GetWorld(), CardSelectionWidgetClass);
        CardSelectionWidget->AddToViewport();
        CardSelectionWidget->BlueprintInitializeWidget();
        CardSelectionViewModel->CardSelectionView = CardSelectionWidget;
        CardSelectionViewModel->OnCardSelectionViewInitialized.Broadcast();
    }
    
    auto* CardViewModel_0 = CardSelectionViewModel->GetCardViewModelByIndex(0);
    if (CardViewModel_0)
    {
        CardViewModel_0->SetUpgradeTag(UpgradeInfos[0].UpgradeEffectTag);
        CardViewModel_0->SetUpgradeDescription(UpgradeInfos[0].UpgradeDescription);
        CardViewModel_0->SetUpgradeName(UpgradeInfos[0].UpgradeName);
        CardViewModel_0->SetMaxStack(UpgradeInfos[0].MaxStack);
        CardViewModel_0->SetUpgradeRarity(UpgradeInfos[0].Rarity);

        CardViewModel_0->OnUpgradeTagAssignedDelegate.Broadcast(UpgradeInfos[0].UpgradeEffectTag);
    }

    auto* CardViewModel_1 = CardSelectionViewModel->GetCardViewModelByIndex(1);
    if (CardViewModel_1)
    {
        CardViewModel_1->SetUpgradeTag(UpgradeInfos[1].UpgradeEffectTag);
        CardViewModel_1->SetUpgradeDescription(UpgradeInfos[1].UpgradeDescription);
        CardViewModel_1->SetUpgradeName(UpgradeInfos[1].UpgradeName);
        CardViewModel_1->SetMaxStack(UpgradeInfos[1].MaxStack);
        CardViewModel_1->SetUpgradeRarity(UpgradeInfos[1].Rarity);
        
        CardViewModel_1->OnUpgradeTagAssignedDelegate.Broadcast(UpgradeInfos[1].UpgradeEffectTag);
    }

    auto* CardViewModel_2 = CardSelectionViewModel->GetCardViewModelByIndex(2);
    if (CardViewModel_2)
    {
        CardViewModel_2->SetUpgradeTag(UpgradeInfos[2].UpgradeEffectTag);
        CardViewModel_2->SetUpgradeDescription(UpgradeInfos[2].UpgradeDescription);
        CardViewModel_2->SetUpgradeName(UpgradeInfos[2].UpgradeName);
        CardViewModel_2->SetMaxStack(UpgradeInfos[2].MaxStack);
        CardViewModel_2->SetUpgradeRarity(UpgradeInfos[2].Rarity);
        
        CardViewModel_2->OnUpgradeTagAssignedDelegate.Broadcast(UpgradeInfos[2].UpgradeEffectTag);
    }
    
    if (auto* AuraPC = Cast<AAuraPlayerController>(GetOwningPlayerController()))
    {
        AuraPC->HandleCardSelectionInitialized();
    }
}

void AAuraHUD::ShowOverlay()
{
    OverlayWidgetController->ShowOverlayWidget(true);
}

void AAuraHUD::HideOverlay()
{
    OverlayWidgetController->ShowOverlayWidget(false);
}
