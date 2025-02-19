// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SpellMenuWidgetController.h"
#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Player/AuraPlayerState.h"

void USpellMenuWidgetController::BroadcastInitialValues()
{
	// 장착된 어빌리티
	BroadcastAbilityInfo();

	// 스펠포인트 초기값 가져오기
	OnSpellPointsChanged.Broadcast(GetAuraPS()->GetSpellPoints());
}

void USpellMenuWidgetController::BindCallbacksToDependencies()
{
	// 어빌리티 상태 변화 델리게이트 바인딩
	GetAuraASC()->AbilityStatusChanged.AddLambda([this](const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag, int32 NewLevel) 
		{
			// 동일한 어빌리티에 대해 어빌리티 상태 업데이트 및 브로드캐스팅
			if (SelectedAbility.Ability.MatchesTagExact(AbilityTag))
			{
				SelectedAbility.Status = StatusTag;

				bool bEnableSpendPoints = false;
				bool bEnableEquip = false;

				// 버튼 활성화
				ShouldEnableButtons(StatusTag, CurrentSpellPoints, bEnableSpendPoints, bEnableEquip);

				// 어빌리티 설명
				FString Description;
				FString NextLevelDescription;
				GetAuraASC()->GetDescriptionsByAbilityTag(AbilityTag, Description, NextLevelDescription);

				// 블루프린트로 브로드캐스팅
				OnSpellGlobeSelected.Broadcast(bEnableSpendPoints, bEnableEquip, Description, NextLevelDescription);
			}

			if (AbilityInfo)
			{
				FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
				Info.StatusTag = StatusTag;
				AbilityInfoDelegate.Broadcast(Info);
			}
		});

	// 어빌리티 장착 델리게이트 바인딩
	GetAuraASC()->AbilityEquipped.AddUObject(this, &USpellMenuWidgetController::OnAbilityEquipped);

	// 스펠 포인트 변화 델리게이트 바인딩
	GetAuraPS()->OnSpellPointChangedDelegate.AddLambda([this](int32 SpellPoints)
		{
			// 블루프린트로 전송
			OnSpellPointsChanged.Broadcast(SpellPoints);
			CurrentSpellPoints = SpellPoints;

			bool bEnableSpendPoints = false;
			bool bEnableEquip = false;

			ShouldEnableButtons(SelectedAbility.Status, CurrentSpellPoints, bEnableSpendPoints, bEnableEquip);

			// 어빌리티 설명
			FString Description;
			FString NextLevelDescription;
			GetAuraASC()->GetDescriptionsByAbilityTag(SelectedAbility.Ability, Description, NextLevelDescription);

			// 블루프린트로 브로드캐스팅
			OnSpellGlobeSelected.Broadcast(bEnableSpendPoints, bEnableEquip, Description, NextLevelDescription);
		});
}

void USpellMenuWidgetController::SpellGlobeSelected(const FGameplayTag& AbilityTag)
{
	if (bWaitForEquipSelection)
	{
		// 스펠 장착 행 강조 애니메이션 중단
		const FGameplayTag SelectedAbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
		StopWaitForEquipDelegate.Broadcast(SelectedAbilityType);
		bWaitForEquipSelection = false;
	}

	// 스펠 포인트, 어빌리티 상태
	const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();
	const int32 SpellPoints = GetAuraPS()->GetSpellPoints();
	FGameplayTag AbilityStatus;

	// (1) 어빌리티 태그가 유효한가
	const bool bTagValid = AbilityTag.IsValid();
	const bool bTagNone = AbilityTag.MatchesTag(GameplayTags.Abilities_None);

	// (2) 활성화 가능한 어빌리티인가
	const FGameplayAbilitySpec* Spec = GetAuraASC()->GetSpecFromAbilityTag(AbilityTag);
	const bool bSpecValid = Spec != nullptr;
	
	// (3) 어빌리티 상태 판단
	if (bTagValid == false || bTagNone == true || bSpecValid == false)
	{
		AbilityStatus = GameplayTags.Abilities_Status_Locked;
	}
	else // 태그가 유효하며 Abilities.None이 아니고 어빌리티 스펙이 유효
	{
		AbilityStatus = GetAuraASC()->GetStatusFromSpec(*Spec);
	}

	// 어빌리티 상태 추적
	SelectedAbility.Ability = AbilityTag;
	SelectedAbility.Status = AbilityStatus;

	// (4) 버튼 활성화
	bool bEnableSpendPoints = false;
	bool bEnableEquip = false;

	ShouldEnableButtons(AbilityStatus, SpellPoints, bEnableSpendPoints, bEnableEquip);

	// 어빌리티 설명
	FString Description;
	FString NextLevelDescription;
	GetAuraASC()->GetDescriptionsByAbilityTag(AbilityTag, Description, NextLevelDescription);

	// 블루프린트로 브로드캐스팅
	OnSpellGlobeSelected.Broadcast(bEnableSpendPoints, bEnableEquip, Description, NextLevelDescription);
}

void USpellMenuWidgetController::SpendPointButtonPressed()
{
	// ASC로 전달
	if (GetAuraASC())
	{
		GetAuraASC()->ServerSpendSpellPoint(SelectedAbility.Ability);
	}
}

void USpellMenuWidgetController::GlobeDeselect()
{
	if (bWaitForEquipSelection)
	{
		// 스펠 장착 행 강조 애니메이션 중단
		const FGameplayTag SelectedAbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
		StopWaitForEquipDelegate.Broadcast(SelectedAbilityType);
		bWaitForEquipSelection = false;
	}

	// 선택된 어빌리티 해제
	SelectedAbility.Ability = FAuraGameplayTags::Get().Abilities_None;
	SelectedAbility.Status = FAuraGameplayTags::Get().Abilities_Status_Locked;

	// 글로브 선택 해제
	OnSpellGlobeSelected.Broadcast(false, false, FString(), FString());
}

void USpellMenuWidgetController::EquipButtonPressed()
{
	// (1) equip 애니메이션 재생
	const FGameplayTag& AbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;

	WaitForEquipDelegate.Broadcast(AbilityType);
	bWaitForEquipSelection = true;

	// 어빌리티 상태 가져오기
	const FGameplayTag SelectedStatus = GetAuraASC()->GetStatusFromAbilityTag(SelectedAbility.Ability);

	// 장착된 어빌리티에 대해 입력 태그 가져오기
	if (SelectedStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
	{
		SelectedSlot = GetAuraASC()->GetInputTagFromAbilityTag(SelectedAbility.Ability);
	}
}

void USpellMenuWidgetController::SpellRowGlobePressed(const FGameplayTag& SlotTag, const FGameplayTag& AbilityType)
{
	if (bWaitForEquipSelection == false)
		return;

	// 액티브 어빌리티를 패시브 칸에 장착할 수 없음, 반대도 동일
	const FGameplayTag& SelectedAbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability).AbilityType;
	if (SelectedAbilityType.MatchesTagExact(AbilityType) == false)
		return;

	// 서버에게 알림
	GetAuraASC()->ServerEquipAbility(SelectedAbility.Ability, SlotTag);
}

void USpellMenuWidgetController::OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PrevSlot)
{
	// 불리언 플래그
	bWaitForEquipSelection = false;

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	FAuraAbilityInfo LastSlotInfo;
	LastSlotInfo.StatusTag = GameplayTags.Abilities_Status_Unlocked;
	LastSlotInfo.InputTag = PrevSlot;
	LastSlotInfo.AbilityTag = GameplayTags.Abilities_None;

	// 변경할 슬롯에 이미 어빌리티가 있다면 빈 어빌리티 정보를 보냄
	AbilityInfoDelegate.Broadcast(LastSlotInfo);

	// 변경할 슬롯에 선택한 어빌리티의 정보를 채움
	FAuraAbilityInfo Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
	Info.StatusTag = Status;
	Info.InputTag = Slot;
	AbilityInfoDelegate.Broadcast(Info);

	// 장착 행 강조 애니메이션 중단
	StopWaitForEquipDelegate.Broadcast(AbilityInfo->FindAbilityInfoForTag(AbilityTag).AbilityType);
	
	// 스펠 글로브 재지정 델리게이트 호출
	SpellGlobeReassignedDelegate.Broadcast(AbilityTag);
	GlobeDeselect();
}

void USpellMenuWidgetController::ShouldEnableButtons(const FGameplayTag& AbilityStatus, int32 SpellPoints, bool& bEnableSpellPointsButton, bool& bEnableEquipButton)
{
	// 밖의 불리언을 가져와 수정할 것이므로 불리언의 참조를 사용한다.
	const FAuraGameplayTags GameplayTags = FAuraGameplayTags::Get();

	// 기본 설정
	bEnableSpellPointsButton = false;
	bEnableEquipButton = false;

	if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Equipped))
	{
		bEnableEquipButton = true;
		if (SpellPoints > 0)
		{
			bEnableSpellPointsButton = true;
		}
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Eligible))
	{
		bEnableEquipButton = false;
		if (SpellPoints > 0)
		{
			bEnableSpellPointsButton = true;
		}
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked))
	{
		bEnableEquipButton = true;
		if (SpellPoints > 0)
		{
			bEnableSpellPointsButton = true;
		}
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Locked))
	{
		bEnableEquipButton = false;
		if (SpellPoints > 0)
		{
			bEnableSpellPointsButton = false;
		}
	}
}
