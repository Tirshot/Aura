// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/HUD/AuraHUD.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Data/AbilityUpgradeInfo.h"
#include "Player/AuraPlayerController.h"
#include "UI/ViewModel/MVVM_AbilityCard.h"
#include "UI/ViewModel/MVVM_CardSelection.h"
#include "UI/Widget/AuraUserWidget.h"
#include "UI/Widget/LoadScreenWidget.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "UI/WidgetController/AttributeMenuWidgetController.h"
#include "UI/WidgetController/SpellMenuWidgetController.h"
#include "UI/WidgetController/GameOverWidgetController.h"
#include "UI/WidgetController/SaveProgressWidgetController.h"

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

UGameOverWidgetController* AAuraHUD::GetGameOverWidgetController(const FWidgetControllerParams& WCParams)
{
    if (GameOverWidgetController == nullptr)
    {   // 없으면 생성
        GameOverWidgetController = NewObject<UGameOverWidgetController>(this, GameOverWidgetControllerClass);
        GameOverWidgetController->SetWidgetControllerParams(WCParams);
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

void AAuraHUD::InitOverlay(APlayerController *PC, APlayerState *PS, UAbilitySystemComponent *ASC, UAttributeSet *AS)
{
    // 위젯과 위젯 컨트롤러 생성
    checkf(OverlayWidgetClass, TEXT("Overlay Widget not Initialized. BP_AuraHUD"));
    checkf(OverlayWidgetControllerClass, TEXT("Overlay Widget Controller not Initialized. BP_AuraHUD"));

    // 위젯 생성 후 오라 유저 위젯으로 캐스팅
    UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
    OverlayWidget = Cast<UAuraUserWidget>(Widget);

    // 구조체에 할당 후 오버레이 위젯 컨트롤러를 초기화
    const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
    UOverlayWidgetController* WidgetController = GetOverlayWidgetController(WidgetControllerParams);

    // 위젯에 위젯 컨트롤러를 연결
    OverlayWidget->SetWidgetController(WidgetController);
    
    // 유효한 속성 세트와 위젯 컨트롤러를 가짐 -> 값 초기화 가능
    WidgetController->BroadcastInitialValues();
    Widget->AddToViewport();
    
    // 게임오버 위젯 컨트롤러 생성
    GameOverWidgetController = GetGameOverWidgetController(WidgetControllerParams);

    // 저장중 위젯 컨트롤러 생성 및 연결
    SaveProgressWidgetController = GetSaveProgressWidgetController(WidgetControllerParams);
    
    // 하드코딩 - 게임 모드가 초기화되면 업그레이드 카드 선택 UI 노출
    OnInitializeGameModeDelegate.AddUObject(this, &AAuraHUD::InitializeCardSelectionUI);
    
    // 플레이어 컨트롤러 초기화 완료 델리게이트 바인딩
    OnInitializePlayerControllerDelegate.AddUObject(this, &AAuraHUD::OnInitializePlayerController);
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

void AAuraHUD::OnInitializePlayerController()
{
    if (AAuraPlayerController* AuraPC = Cast<AAuraPlayerController>(GetOwningPlayerController()))
    {
        AuraPC->OnCardSelectionInitializedDelegate.Broadcast();
    }
}

void AAuraHUD::InitializeCardSelectionUI()
{
    // 뷰 모델과 뷰 생성
    CardSelectionViewModel = NewObject<UMVVM_CardSelection>(this, CardSelectionViewModelClass);
    CardSelectionViewModel->InitializeSlot();

    CardSelectionWidget = CreateWidget<ULoadScreenWidget>(GetWorld(), CardSelectionWidgetClass);
    CardSelectionWidget->AddToViewport();

    CardSelectionWidget->BlueprintInitializeWidget();

    //
    ReceivedCardsDelegate.AddUObject(this, &AAuraHUD::HandleRandomAbilityUpgrade);
    
    // 게임모드의 델리게이트 호출 -> 카드를 이니셜라이즈
    InitializeCardsDelegate.Broadcast(GetOwningPlayerController());
}

void AAuraHUD::HandleRandomAbilityUpgrade(FGameplayTag UpgradeTag0, FGameplayTag UpgradeTag1,
    FGameplayTag UpgradeTag2)
{
    auto* Info = UAuraAbilitySystemLibrary::GetAbilityUpgradeInfo(this);
    if (Info == nullptr)
        return;
    
    auto* CardViewModel_0 = CardSelectionViewModel->GetCardViewModelByIndex(0);
    if (CardViewModel_0)
    {
        auto Upgrade = Info->GetUpgradeInfoForUpgradeTag(UpgradeTag0);
        
        CardViewModel_0->SetUpgradeTag(Upgrade.UpgradeEffectTag);
        CardViewModel_0->SetUpgradeDescription(Upgrade.UpgradeDescription);
        CardViewModel_0->SetUpgradeName(Upgrade.UpgradeName);
        CardViewModel_0->SetUpgradeMaxLevel(Upgrade.UpgradeMaxLevel);

        CardViewModel_0->OnUpgradeTagAssignedDelegate.Broadcast(UpgradeTag0);
    }

    auto* CardViewModel_1 = CardSelectionViewModel->GetCardViewModelByIndex(1);
    if (CardViewModel_1)
    {
        auto Upgrade = Info->GetUpgradeInfoForUpgradeTag(UpgradeTag1);
        
        CardViewModel_1->SetUpgradeTag(Upgrade.UpgradeEffectTag);
        CardViewModel_1->SetUpgradeDescription(Upgrade.UpgradeDescription);
        CardViewModel_1->SetUpgradeName(Upgrade.UpgradeName);
        CardViewModel_1->SetUpgradeMaxLevel(Upgrade.UpgradeMaxLevel);
        
        CardViewModel_1->OnUpgradeTagAssignedDelegate.Broadcast(UpgradeTag1);
    }

    auto* CardViewModel_2 = CardSelectionViewModel->GetCardViewModelByIndex(2);
    if (CardViewModel_2)
    {
        auto Upgrade = Info->GetUpgradeInfoForUpgradeTag(UpgradeTag2);
        
        CardViewModel_2->SetUpgradeTag(Upgrade.UpgradeEffectTag);
        CardViewModel_2->SetUpgradeDescription(Upgrade.UpgradeDescription);
        CardViewModel_2->SetUpgradeName(Upgrade.UpgradeName);
        CardViewModel_2->SetUpgradeMaxLevel(Upgrade.UpgradeMaxLevel);
        
        CardViewModel_2->OnUpgradeTagAssignedDelegate.Broadcast(UpgradeTag2);
    }
}