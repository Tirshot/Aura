// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SpellMenuWidgetController.h"

#include "AbilitySystem/AuraAbilitySystemComponent.h"
#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "AbilitySystem/Abilities/AuraPassiveAbility.h"
#include "AbilitySystem/Data/AbilityInfo.h"
#include "Character/AuraCharacter.h"
#include "Interaction/CombatInterface.h"
#include "Player/AuraPlayerState.h"

void USpellMenuWidgetController::BroadcastInitialValues()
{
	BroadcastAbilityInfo();

	OnSpellPointsChanged.Broadcast(GetAuraPS()->GetSpellPoints());
	
	// 창을 껐다 키면 지금의 장착 상태를 다시 가져옴
	if (UAuraAbilitySystemComponent* AuraASC = GetAuraASC())
	{
		// 현재 장착된 모든 어빌리티 슬롯 정보 재브로드캐스트
		for (const FGameplayAbilitySpec& Spec : AuraASC->GetActivatableAbilities())
		{
			const FGameplayTag AbilityTag = AuraASC->GetAbilityTagFromSpec(Spec);
			const FGameplayTag StatusTag = AuraASC->GetStatusFromSpec(Spec);
			const FGameplayTag SlotTag = AuraASC->GetInputTagFromSpec(Spec);

			if (!AbilityTag.IsValid() || !SlotTag.IsValid())
				continue;
			
			// 장착된 어빌리티만
			if (!StatusTag.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
				continue;

			if (FAuraAbilityInfo* Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag))
			{
				Info->StatusTag = StatusTag;
				Info->InputTag = SlotTag;
				AbilityInfoDelegate.Broadcast(*Info);
			}
		}
	}
}

void USpellMenuWidgetController::BindCallbacksToDependencies()
{
	// 어빌리티 상태가 변경
	GetAuraASC()->AbilityStatusChanged.AddDynamic(this, &USpellMenuWidgetController::OnAbilityStatusChanged);

	// 어빌리티 장착
	GetAuraASC()->AbilityEquipped.AddDynamic(this, &USpellMenuWidgetController::OnAbilityEquipped);

	// 스펠 포인트 변경
	GetAuraPS()->OnSpellPointChangedDelegate.AddDynamic(this, &USpellMenuWidgetController::OnSpellPointChanged);
}

void USpellMenuWidgetController::OnSpellPointChanged(int32 SpellPoints)
{
	OnSpellPointsChanged.Broadcast(SpellPoints);
	CurrentSpellPoints = SpellPoints;

	bool bEnableSpendPoints = false;
	bool bEnableEquip = false;
		
	// 패시브 어빌리티일 경우 최대 레벨 이상 찍을 수 없음
	if (SelectedAbility.Ability.MatchesTag(FAuraGameplayTags::Get().Abilities_Passive))
	{
		if (auto* Spec = GetAuraASC()->GetSpecFromAbilityTag(SelectedAbility.Ability))
		{
			int32 MaxLevel = 1;
			if (UAuraPassiveAbility* Passive = Cast<UAuraPassiveAbility>(Spec->Ability.Get()))
			{
				MaxLevel = Passive->GetMaxLevel();
			}
									
			int32 CurrentLevel = Spec->Level;
			bool bIsExceedMaxLevel = CurrentLevel >= MaxLevel;
			ShouldEnableButtons(SelectedAbility.Status, CurrentSpellPoints, bEnableSpendPoints, bEnableEquip, bIsExceedMaxLevel);
		}
	}
	else
	{
		ShouldEnableButtons(SelectedAbility.Status, CurrentSpellPoints, bEnableSpendPoints, bEnableEquip, false);
	}
		
	FString Description;
	FString NextLevelDescription;
	GetAuraASC()->GetDescriptionsByAbilityTag(SelectedAbility.Ability, Description, NextLevelDescription);

	OnSpellGlobeSelected.Broadcast(bEnableSpendPoints, bEnableEquip, Description, NextLevelDescription);
}

void USpellMenuWidgetController::OnAbilityStatusChanged(const FGameplayTag& AbilityTag, const FGameplayTag& StatusTag,
	int32 NewLevel)
{
	// 어빌리티를 찾아 UI에 반영
	if (SelectedAbility.Ability.MatchesTagExact(AbilityTag))
	{
		SelectedAbility.Status = StatusTag;

		bool bEnableSpendPoints = false;
		bool bEnableEquip = false;
				
		// 버튼 활성화 확인
		// 패시브 어빌리티일 경우 최대 레벨 이상 찍을 수 없음
		if (AbilityTag.MatchesTag(FAuraGameplayTags::Get().Abilities_Passive))
		{
			if (auto* Spec = GetAuraASC()->GetSpecFromAbilityTag(AbilityTag))
			{
				int32 MaxLevel = 1;
				if (UAuraPassiveAbility* Passive = Cast<UAuraPassiveAbility>(Spec->Ability.Get()))
				{
					MaxLevel = Passive->GetMaxLevel();
				}
						
				int32 CurrentLevel = Spec->Level;
				bool bIsExceedMaxLevel = CurrentLevel >= MaxLevel;
				ShouldEnableButtons(StatusTag, CurrentSpellPoints, bEnableSpendPoints, bEnableEquip, bIsExceedMaxLevel);
			}
		}
		else
		{
			ShouldEnableButtons(StatusTag, CurrentSpellPoints, bEnableSpendPoints, bEnableEquip, false);
		}

		FString Description;
		FString NextLevelDescription;
		GetAuraASC()->GetDescriptionsByAbilityTag(AbilityTag, Description, NextLevelDescription);

		// 글로브 선택 상태 변경
		OnSpellGlobeSelected.Broadcast(bEnableSpendPoints, bEnableEquip, Description, NextLevelDescription);
	}

	if (AbilityInfo)
	{
		FAuraAbilityInfo* Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag);
		if (!Info)
			return;
		
		Info->StatusTag = StatusTag;
		AbilityInfoDelegate.Broadcast(*Info);
	}
}

void USpellMenuWidgetController::SpellGlobeSelected(const FGameplayTag& AbilityTag)
{
	if (bWaitForEquipSelection)
	{
		const FGameplayTag SelectedAbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability)->AbilityType;
		StopWaitForEquipDelegate.Broadcast(SelectedAbilityType);
		bWaitForEquipSelection = false;
	}

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();
	const int32 SpellPoints = GetAuraPS()->GetSpellPoints();
	FGameplayTag AbilityStatus;

	const bool bTagValid = AbilityTag.IsValid();
	const bool bTagNone = AbilityTag.MatchesTag(GameplayTags.Abilities_None);

	const FGameplayAbilitySpec* Spec = GetAuraASC()->GetSpecFromAbilityTag(AbilityTag);
	const bool bSpecValid = Spec != nullptr;
	
	if (bTagValid == false || bTagNone == true || bSpecValid == false)
	{
		AbilityStatus = GameplayTags.Abilities_Status_Locked;
	}
	else
	{
		AbilityStatus = GetAuraASC()->GetStatusFromSpec(*Spec);
	}

	SelectedAbility.Ability = AbilityTag;
	SelectedAbility.Status = AbilityStatus;

	bool bEnableSpendPoints = false;
	bool bEnableEquip = false;
	
	// 패시브 어빌리티일 경우 최대 레벨 이상 찍을 수 없음
	if (AbilityTag.MatchesTag(FAuraGameplayTags::Get().Abilities_Passive))
	{
		if (Spec)
		{
			int32 MaxLevel = 1;
			if (UAuraPassiveAbility* Passive = Cast<UAuraPassiveAbility>(Spec->Ability.Get()))
			{
				MaxLevel = Passive->GetMaxLevel();
			}
						
			int32 CurrentLevel = Spec->Level;
			bool bIsExceedMaxLevel = CurrentLevel >= MaxLevel;
			ShouldEnableButtons(AbilityStatus, CurrentSpellPoints, bEnableSpendPoints, bEnableEquip, bIsExceedMaxLevel);
		}
	}
	else
	{
		ShouldEnableButtons(AbilityStatus, CurrentSpellPoints, bEnableSpendPoints, bEnableEquip, false);
	}
	
	FString Description;
	FString NextLevelDescription;
	GetAuraASC()->GetDescriptionsByAbilityTag(AbilityTag, Description, NextLevelDescription);

	// 어빌리티 레벨이 0보다 적으면 장착 불가
	if (Spec && Spec->Level <= 0)
		bEnableEquip = false;
	
	OnSpellGlobeSelected.Broadcast(bEnableSpendPoints, bEnableEquip, Description, NextLevelDescription);
}

void USpellMenuWidgetController::SpendPointButtonPressed()
{
	if (UAuraAbilitySystemComponent* AuraASC = GetAuraASC())
	{
		// 튜토리얼 조건 : 감전사에 포인트 투자
		if (UAuraAbilitySystemLibrary::IsThisMapTutorial(this) && SelectedAbility.Ability != FAuraGameplayTags::Get().Abilities_Lightning_Electrocute)
			return;
		
		// 요구 조건과 맞지 않으면 스펠 포인트 투자 불가
		for (FAuraAbilityInfo& Info : AbilityInfo->AbilityInformation)
		{
			FGameplayTag AbilityTag = Info.AbilityTag;
			
			if (AbilityTag == SelectedAbility.Ability)
			{
				FGameplayTag InferiorAbilityTag = Info.RequireInferiorAbilityTag;
				if (InferiorAbilityTag.MatchesTag(FAuraGameplayTags::Get().Abilities_None))
				{
					// 하위 스펠 제한조건이 없다면 레벨 조건 만족 시 스펠 포인트 소모 가능
					if (AActor* AvatarActor = AuraAbilitySystemComponent->GetAvatarActor())
					{
						int32 PlayerLevel= ICombatInterface::Execute_GetCharacterLevel(AvatarActor);
						int32 LevelReq = Info.LevelRequirement;
									
						if (PlayerLevel >= LevelReq)
						{
							AuraASC->ServerSpendSpellPoint(SelectedAbility.Ability);
							return;
						}
						UAuraAbilitySystemLibrary::AddMessageToActor(AvatarActor, FGameplayTag::RequestGameplayTag("Message.NotEnoughLevel"), FText::AsNumber(LevelReq));
					}
				}
				else
				{
					// 하위 스펠 제한조건이 있으면
					for (FAuraAbilityInfo& InferiorInfo : AbilityInfo->AbilityInformation)
					{
						// 하위 스펠을 익혔을 때
						if (InferiorInfo.AbilityTag.MatchesTagExact(InferiorAbilityTag))
						{
							// 레벨 조건 만족 시
							if (AActor* AvatarActor = AuraAbilitySystemComponent->GetAvatarActor())
							{
								int32 PlayerLevel= ICombatInterface::Execute_GetCharacterLevel(AvatarActor);
								int32 LevelReq = Info.LevelRequirement;
									
								if (PlayerLevel >= LevelReq)
								{
									AuraASC->ServerSpendSpellPoint(SelectedAbility.Ability);
									return;
								}
								UAuraAbilitySystemLibrary::AddMessageToActor(Cast<AAuraCharacter>(AvatarActor), FGameplayTag::RequestGameplayTag("Message.NotEnoughLevel"), FText::AsNumber(LevelReq));
							}
						}
					}
					if (AActor* AvatarActor = AuraAbilitySystemComponent->GetAvatarActor())
					{
						UAuraAbilitySystemLibrary::AddMessageToActor(Cast<AAuraCharacter>(AvatarActor), FGameplayTag::RequestGameplayTag("Message.NotEnoughInferiorSpellLevel"));
					}
				}
			}
		}
	}
}

void USpellMenuWidgetController::GlobeDeselect()
{
	if (bWaitForEquipSelection)
	{
		const FGameplayTag SelectedAbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability)->AbilityType;
		StopWaitForEquipDelegate.Broadcast(SelectedAbilityType);
		bWaitForEquipSelection = false;
	}

	SelectedAbility.Ability = FAuraGameplayTags::Get().Abilities_None;
	SelectedAbility.Status = FAuraGameplayTags::Get().Abilities_Status_Locked;

	OnSpellGlobeSelected.Broadcast(false, false, FString(), FString());
}

void USpellMenuWidgetController::EquipButtonPressed()
{
	const FGameplayTag& AbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability)->AbilityType;

	WaitForEquipDelegate.Broadcast(AbilityType);
	bWaitForEquipSelection = true;

	const FGameplayTag SelectedStatus = GetAuraASC()->GetStatusFromAbilityTag(SelectedAbility.Ability);

	if (SelectedStatus.MatchesTagExact(FAuraGameplayTags::Get().Abilities_Status_Equipped))
	{
		SelectedSlot = GetAuraASC()->GetSlotFromAbilityTag(SelectedAbility.Ability);
	}
}

void USpellMenuWidgetController::SpellRowGlobePressed(const FGameplayTag& SlotTag, const FGameplayTag& AbilityType)
{
	if (bWaitForEquipSelection == false)
		return;

	const FGameplayTag& SelectedAbilityType = AbilityInfo->FindAbilityInfoForTag(SelectedAbility.Ability)->AbilityType;
	if (SelectedAbilityType.MatchesTagExact(AbilityType) == false)
		return;

	GetAuraASC()->ServerEquipAbility(SelectedAbility.Ability, SlotTag);
	
	// 튜토리얼 조건 : 감전사 장착
	if (UAuraAbilitySystemLibrary::IsThisMapTutorial(this) && SelectedAbility.Ability.MatchesTag(FAuraGameplayTags::Get().Abilities_Lightning_Electrocute))
	{
		// 튜토리얼 조건 완료
		ElectrocuteAssignedDelegate.Broadcast(SelectedAbility.Ability);
		ElectrocuteAssignedDelegate.Clear();
	}
}

void USpellMenuWidgetController::OnAbilityEquipped(const FGameplayTag& AbilityTag, const FGameplayTag& Status, const FGameplayTag& Slot, const FGameplayTag& PrevSlot)
{
	bWaitForEquipSelection = false;

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	if (PrevSlot.IsValid() && !PrevSlot.MatchesTagExact(GameplayTags.Abilities_None))
	{
		FAuraAbilityInfo LastSlotInfo;
		LastSlotInfo.StatusTag = GameplayTags.Abilities_Status_Unlocked;
		LastSlotInfo.InputTag = PrevSlot;
		LastSlotInfo.AbilityTag = GameplayTags.Abilities_None;
		AbilityInfoDelegate.Broadcast(LastSlotInfo);
	}

	if (!AbilityTag.IsValid())
		return;
	
	if (FAuraAbilityInfo* Info = AbilityInfo->FindAbilityInfoForTag(AbilityTag))
	{
		Info->StatusTag = Status;
		Info->InputTag = Slot;
		Info->AbilityTag = AbilityTag;
		AbilityInfoDelegate.Broadcast(*Info);
	}
	
	StopWaitForEquipDelegate.Broadcast(AbilityInfo->FindAbilityInfoForTag(AbilityTag)->AbilityType);
	
	SpellGlobeReassignedDelegate.Broadcast(AbilityTag);
	GlobeDeselect();
}
void USpellMenuWidgetController::ShouldEnableButtons(const FGameplayTag& AbilityStatus, int32 SpellPoints, bool& bEnableSpellPointsButton, bool& bEnableEquipButton, bool bIsPassiveLevelExceeded)
{
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	bEnableSpellPointsButton = false;
	bEnableEquipButton = false;

	if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Equipped))
	{
		bEnableEquipButton = true;
		if (SpellPoints > 0)
		{
			bEnableSpellPointsButton = true;
			if (bIsPassiveLevelExceeded)
			{
				bEnableSpellPointsButton = false;
			}
		}
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Eligible))
	{
		bEnableEquipButton = false;
		if (SpellPoints > 0)
		{
			bEnableSpellPointsButton = true;
			if (bIsPassiveLevelExceeded)
			{
				bEnableSpellPointsButton = false;
			}
		}
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Unlocked))
	{
		bEnableEquipButton = true;
		if (SpellPoints > 0)
		{
			bEnableSpellPointsButton = true;
			if (bIsPassiveLevelExceeded)
			{
				bEnableSpellPointsButton = false;
			}
		}
	}
	else if (AbilityStatus.MatchesTagExact(GameplayTags.Abilities_Status_Locked))
	{
		bEnableSpellPointsButton = false;
		bEnableEquipButton = false;
	}
}
