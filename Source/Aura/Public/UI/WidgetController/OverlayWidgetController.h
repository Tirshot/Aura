// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "OverlayWidgetController.generated.h"

struct FInventorySlot;
class UAuraUserWidget;
class UAbilityInfo;
class UAuraAbilitySystemComponent;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EMessageType : uint8
{
	Floating,
	TextBox,
	SpellDesc
};

USTRUCT(BlueprintType)
struct FUIWidgetRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGameplayTag MessageTag = FGameplayTag();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine = true))
	FText Message = FText();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<class UAuraUserWidget> MessageWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	UTexture2D* Image = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EMessageType MessageType = EMessageType::Floating;
};

// 동적 멀티캐스트 - dynamic multicast delegate
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerStatChangedSignature, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelChangedSignature, int32, NewValue, bool, bLevelUp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangedSignature, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMessageWidgetRowSignature, FUIWidgetRow, Row);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMessageRemoveSignature, const FGameplayTag&, MessageTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAbilityActivatedSignature, bool, bAbilityActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOverlayVisibilityChangedSignature, bool, bVisibility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnButtonVisibilityChangedSignature, bool, bVisibility);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCloseMenuAnchors, bool, bClose);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDebugModeActivated, bool, bActivated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShowMenuKeyPressed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOpenMenuAnchor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCenterDescriptionRemoved);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemToolTipActivated, const FName&, ItemName, bool, bActivated);

UCLASS(BlueprintType, Blueprintable)
class AURA_API UOverlayWidgetController : public UAuraWidgetController
{
	GENERATED_BODY()

public:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacksToDependencies() override;

	// 델리게이트일 경우에 Assignable
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeChangedSignature OnMaxHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeChangedSignature OnManaChanged;

	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeChangedSignature OnMaxManaChanged;

	UPROPERTY(BlueprintAssignable, Category="GAS|Messages")
	FMessageWidgetRowSignature MessageWidgetRowDelegate;

	UPROPERTY(BlueprintAssignable, Category = "GAS|XP")
	FOnAttributeChangedSignature OnXPPercentChanged;

	UPROPERTY(BlueprintAssignable, Category = "GAS|Messages")
	FOnLevelChangedSignature OnPlayerLevelChangedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "GAS|Messages")
	FOnMessageRemoveSignature OnMessageRemoved;

	UPROPERTY(BlueprintAssignable, Category = "OverlayWidget")
	FOnOverlayVisibilityChangedSignature OnOverlayVisibilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "OverlayWidget")
	FOnButtonVisibilityChangedSignature OnButtonVisibilityChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "OverlayWidget")
	FOnButtonVisibilityChangedSignature OnAttributeMenuButtonVisibilityChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "OverlayWidget")
	FOnButtonVisibilityChangedSignature OnSpellMenuButtonVisibilityChanged;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnButtonVisibilityChangedSignature OnLevelUpButtonVisibilityChanged;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnButtonVisibilityChangedSignature OnNextButtonVisibilityChanged;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnCloseMenuAnchors OnCloseMenuAnchors;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnDebugModeActivated OnDebugModeActivated;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnShowMenuKeyPressed OnAttributeMenuKeyPressed;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnShowMenuKeyPressed OnSpellMenuKeyPressed;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnShowMenuKeyPressed OnESCMenuKeyPressed;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnShowMenuKeyPressed OnInventoryMenuKeyPressed;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnOpenMenuAnchor OnSettingsMenuAnchorOpen;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnOpenMenuAnchor OnDebugMenuAnchorOpen;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnOpenMenuAnchor OnAreYouSureAnchorOpen;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnItemToolTipActivated OnItemToolTipActivated;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "OverlayWidget")
	FOnCenterDescriptionRemoved OnCenterDescriptionRemoved;
	
public:
	UFUNCTION(BlueprintCallable)
	void ShowOverlayWidget(bool bShow);

	UFUNCTION(BlueprintCallable)
	void ShowOverlayButtons(bool bShow);

	UFUNCTION(BlueprintCallable)
	void ShowAttributeMenuButton(bool bShow);
	
	UFUNCTION(BlueprintCallable)
	void ShowSpellMenuButton(bool bShow);

public:
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UUserWidget> OverlayWidget;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Widget Data")
	TObjectPtr<UDataTable> MessageWidgetDataTable;

	template<typename T>
	T* GetDataTableRowByTag(UDataTable* DataTable, const FGameplayTag& Tag);

public:
	template<typename T>
	T* GetDataTableRowByTagFromMemberTable(const FGameplayTag& Tag)
	{
		return MessageWidgetDataTable->FindRow<T>(Tag.GetTagName(), TEXT(""));
	}

protected:
	// 경험치 콜백 함수
	UFUNCTION()
	void OnXPChanged(int32 NewXP);

	// 어빌리티 장착 표시
	void OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PrevSlot);

	// 메시지 제거
	UFUNCTION()
	void MessageRemove(const FGameplayTag& Tag);
	
public:
	UFUNCTION(BlueprintCallable)
	void RemoveCenterDescriptionMessage();
};

template <typename T>
inline T *UOverlayWidgetController::GetDataTableRowByTag(UDataTable *DataTable, const FGameplayTag &Tag)
{
	return DataTable->FindRow<T>(Tag.GetTagName(), TEXT(""));
}
