// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AsyncTasks/WaitCooldownChange.h"
#include "AbilitySystemComponent.h"

UWaitCooldownChange* UWaitCooldownChange::WaitForCooldownChange(UAbilitySystemComponent* AbilitySystemComponent, const FGameplayTag& InCooldownTag)
{
	UWaitCooldownChange* WaitCooldownChange = NewObject<UWaitCooldownChange>();
	WaitCooldownChange->ASC = AbilitySystemComponent;
	WaitCooldownChange->CooldownTag = InCooldownTag;

	// ��ȿ���� ������ null ��ȯ
	if (IsValid(AbilitySystemComponent) == false || InCooldownTag.IsValid() == false)
	{
		WaitCooldownChange->EndTask();
		return nullptr;
	}

	// ��ٿ��� ���� �����°�(�±װ� ���� ������°�)
	AbilitySystemComponent->RegisterGameplayTagEvent(
		InCooldownTag,	EGameplayTagEventType::NewOrRemoved).AddUObject(
			WaitCooldownChange,
			&UWaitCooldownChange::CooldownTagChanged);

	// ��ٿ��� ���� ����Ǿ��°�
	AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(WaitCooldownChange, &UWaitCooldownChange::OnActiveEffectAdded);

	return WaitCooldownChange;
}

void UWaitCooldownChange::EndTask()
{
	if (IsValid(ASC) == false)
		return;
	
	// �ݹ� �Լ� ���ε� ����
	ASC->RegisterGameplayTagEvent(CooldownTag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
	
	SetReadyToDestroy();

	// �������� ǥ��
	MarkAsGarbage();
}

void UWaitCooldownChange::CooldownTagChanged(const FGameplayTag InCooldownTag, int32 NewCount)
{
	// ��ٿ� ����
	if (NewCount == 0)
	{
		CooldownEnd.Broadcast(0.f);
	}
}

void UWaitCooldownChange::OnActiveEffectAdded(UAbilitySystemComponent* TargetASC, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer AssetTags;
	SpecApplied.GetAllAssetTags(AssetTags);

	FGameplayTagContainer GrantedTags;
	SpecApplied.GetAllGrantedTags(GrantedTags);

	// �±׸� ã�� ��ٿ� �±װ� �ο��Ǿ� �ִٸ�
	if (AssetTags.HasTagExact(CooldownTag) || GrantedTags.HasTagExact(CooldownTag))
	{
		// �ش��ϴ� ��ٿ� �±׸� ������ ����Ʈ ã��
		FGameplayEffectQuery GameplayEffectQuery = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTag.GetSingleTagContainer());
		
		// �ټ��� ��ٿ��� ���� �� ����
		TArray<float> TimesRemaining = ASC->GetActiveEffectsTimeRemaining(GameplayEffectQuery);
		if (TimesRemaining.Num() > 0)
		{
			// ��ٿ� �� �ִ�(���� �����ɸ��� �ð�) ã��
			float TimeRemaining = TimesRemaining[0];
			for (int32 i = 0; i < TimesRemaining.Num(); i++)
			{
				if (TimeRemaining < TimesRemaining[i])
				{
					TimeRemaining = TimesRemaining[i];
				}
			}

			// ��ٿ� �ð� ��ε�ĳ��Ʈ
			CooldownStart.Broadcast(TimeRemaining);
		}
	}
}