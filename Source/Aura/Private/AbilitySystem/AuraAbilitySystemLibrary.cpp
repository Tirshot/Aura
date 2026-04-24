// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AuraAbilitySystemLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AuraAbilityTypes.h"
#include "AuraGameplayTags.h"
#include "SocketSubsystem.h"
#include "AbilitySystem/AuraAttributeSet.h"
#include "Character/AuraCharacter.h"
#include "Character/AuraEnemy.h"
#include "Game/AuraGameModeBase.h"
#include "Game/LoadScreenSaveGame.h"
#include "Interaction/CombatInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Player/AuraPlayerState.h"
#include "UI/HUD/AuraHUD.h"
#include "UI/WidgetController/AuraWidgetController.h"
#include "Engine/OverlapResult.h"
#include "Game/AuraGameInstance.h"
#include "Player/AuraPlayerController.h"
#include "UI/HUD/LoadScreenHUD.h"
#include "UI/Widget/AuraCenterDescriptionWidget.h"
#include "UI/WidgetController/SpellUpgradesWidgetController.h"

bool UAuraAbilitySystemLibrary::MakeWidgetControllerParams(const UObject* WorldContextObject, FWidgetControllerParams& OutWCParams, AAuraHUD*& OutAuraHUD)
{
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		OutAuraHUD = Cast<AAuraHUD>(PC->GetHUD());
		if (OutAuraHUD)
		{
			// PS, ASC, AS 가져오기
			if (AAuraPlayerState* PS = PC->GetPlayerState<AAuraPlayerState>())
			{
				UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
				UAttributeSet* AS = PS->GetAttributeSet();

				// 파라미터 구조체를 채워서 내보내기
				OutWCParams.PlayerState = PS;
				OutWCParams.AbilitySystemComponent = ASC;
				OutWCParams.AttributeSet = AS;

				return true;
			}
		}
	}
	return false;
}

UOverlayWidgetController* UAuraAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;

	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetOverlayWidgetController(WCParams);
	}
	return nullptr;
}

UAttributeMenuWidgetController* UAuraAbilitySystemLibrary::GetAttributeMenuWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;

	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetAttributeMenuWidgetController(WCParams);
	}
	return nullptr;
}

USpellMenuWidgetController* UAuraAbilitySystemLibrary::GetSpellMenuWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;

	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetSpellMenuWidgetController(WCParams);
	}
	return nullptr;
}

USpellUpgradesWidgetController* UAuraAbilitySystemLibrary::GetSpellUpgradesWidgetController(
	const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;

	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetSpellUpgradesWidgetController(WCParams);
	}
	return nullptr;
}

UGameOverWidgetController* UAuraAbilitySystemLibrary::GetGameOverWidgetController(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;

	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetGameOverWidgetController(WCParams);
	}
	return nullptr;
}

USettingsMenuWidgetController* UAuraAbilitySystemLibrary::GetSettingsMenuWidgetController(
	const UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return nullptr;
	
	if (UAuraGameInstance* AuraGI = WorldContextObject->GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		return AuraGI->GetSettingsMenuWidgetController();
	}
	return nullptr;
}

UItemToolTipWidgetController* UAuraAbilitySystemLibrary::GetItemToolTipWidgetController(
	const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;

	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetItemToolTipWidgetController(WCParams);
	}
	return nullptr;
}

UMVVM_DebugMenu* UAuraAbilitySystemLibrary::GetDebugMenuViewModel(const UObject* WorldContextObject)
{
	FWidgetControllerParams WCParams;
	AAuraHUD* AuraHUD = nullptr;
	if (MakeWidgetControllerParams(WorldContextObject, WCParams, AuraHUD))
	{
		return AuraHUD->GetDebugMenuViewModel(WCParams);
	}
	return nullptr;
}

UMVVM_CardSelection* UAuraAbilitySystemLibrary::GetCardSelectionViewModel(const UObject* WorldContextObject)
{
	AAuraHUD* AuraHUD = nullptr;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		AuraHUD = Cast<AAuraHUD>(PC->GetHUD());
		if (AuraHUD)
		{
			return AuraHUD->CardSelectionViewModel;
		}
	}

	return nullptr;
}

void UAuraAbilitySystemLibrary::InitializeDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	AActor* AvatarActor = ASC->GetAvatarActor();
	AAuraCharacterBase* AvatarCharacter = Cast<AAuraCharacterBase>(AvatarActor);
	
	// 액터의 클래스 정보 가져오기
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(AvatarActor);
	if (CharacterClassInfo == nullptr)
		return;

	FCharacterClassDefaultInfo ClassDefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);

	

	// 이펙트 스펙을 위한 컨텍스트 핸들 생성
	FGameplayEffectContextHandle PrimaryAttributesContextHandle = ASC->MakeEffectContext();
	PrimaryAttributesContextHandle.AddSourceObject(AvatarActor);

	FGameplayEffectContextHandle SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(AvatarActor);

	FGameplayEffectContextHandle VitalAttributesContextHandle = ASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(AvatarActor);

	// 이펙트 적용을 위한 이펙트 스펙 생성 후 적용 - ClassClassInfo가 아닌 클래스 디폴트를 참조하도록 수정됨
	const FGameplayEffectSpecHandle PrimaryAttributesSpecHandle = ASC->MakeOutgoingSpec(AvatarCharacter->DefaultPrimaryAttributes, Level, PrimaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*PrimaryAttributesSpecHandle.Data.Get());

	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(AvatarCharacter->DefaultSecondaryAttributes, Level, SecondaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes, Level, VitalAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data.Get());
}

void UAuraAbilitySystemLibrary::InitializeDefaultAttributesFromSaveData(const UObject* WorldContextObject,
	UAbilitySystemComponent* ASC, ULoadScreenSaveGame* SaveGame)
{
	// 액터의 클래스 정보 가져오기
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr)
		return;

	// Set by Caller 가져오기
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	const AActor* SourceActor = ASC->GetAvatarActor();

	// 이펙트 컨텍스트 핸들 생성
	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(SourceActor);

	// 이펙트 적용을 위한 이펙트 스펙 핸들 생성
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->PrimaryAttributes_SetByCaller, 1.f, EffectContextHandle);

	// Set By Caller Magnitude 설정
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Strength, SaveGame->Strength);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Intelligence, SaveGame->Intelligence);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Resilience, SaveGame->Resilience);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Vigor, SaveGame->Vigor);

	// 게임플레이 이펙트 적용
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 2차 속성 적용
	FGameplayEffectContextHandle SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttributes, 1.f, SecondaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

	// 세이브 데이터의 바이탈 속성 적용
	FGameplayEffectContextHandle VitalAttributesContextHandle = ASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes_SetByCaller, 1.f, VitalAttributesContextHandle);
	
	float MaxHealth = GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth);
	float MaxMana = GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MaxMana);
	
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(VitalAttributesSpecHandle, GameplayTags.Attributes_Vital_Health, SaveGame->Health * MaxHealth);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(VitalAttributesSpecHandle, GameplayTags.Attributes_Vital_Mana, SaveGame->Mana * MaxMana);

	ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data.Get());

	// 세이브 데이터의 리젠 속성 적용
	FGameplayEffectContextHandle RegenAttributesContextHandle = ASC->MakeEffectContext();
	RegenAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle RegenAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->RegenAttributes, 1.f, RegenAttributesContextHandle);

	ASC->ApplyGameplayEffectSpecToSelf(*RegenAttributesSpecHandle.Data.Get());
}

void UAuraAbilitySystemLibrary::InitializeDefaultAttributesFromAttributes(const UObject* WorldContextObject,
	UAbilitySystemComponent* ASC, float Strength, float Intelligence, float Vigor, float Resilience, float Health, float Mana)
{
	UE_LOG(LogTemp, Warning, TEXT("InitializeAttributes: Called on Server for %s"), *ASC->GetAvatarActor()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("Passed Data - Strength: %f, Health: %f, Mana: %f"), Strength, Health, Mana);
	
	// 액터의 클래스 정보 가져오기
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr)
		return;

	// Set by Caller 가져오기
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	const AActor* SourceActor = ASC->GetAvatarActor();

	// 이펙트 컨텍스트 핸들 생성
	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(SourceActor);

	// 이펙트 적용을 위한 이펙트 스펙 핸들 생성
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->PrimaryAttributes_SetByCaller, 1.f, EffectContextHandle);

	// Set By Caller Magnitude 설정
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Strength, Strength);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Intelligence, Intelligence);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Resilience, Resilience);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Vigor, Vigor);

	// 게임플레이 이펙트 적용
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 2차 속성 적용
	FGameplayEffectContextHandle SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttributes, 1.f, SecondaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

	// 세이브 데이터의 바이탈 속성 적용
	FGameplayEffectContextHandle VitalAttributesContextHandle = ASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes_SetByCaller, 1.f, VitalAttributesContextHandle);
	
	// 바이탈 속성, 비율로 저장
	float MaxHealth = GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MaxHealth);
	float MaxMana = GetAttributeValue(WorldContextObject, FAuraGameplayTags::Get().Attributes_Secondary_MaxMana);
	
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(VitalAttributesSpecHandle, GameplayTags.Attributes_Vital_Health, MaxHealth * Health);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(VitalAttributesSpecHandle, GameplayTags.Attributes_Vital_Mana, MaxMana * Mana);

	ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data.Get());

	// 세이브 데이터의 리젠 속성 적용
	FGameplayEffectContextHandle RegenAttributesContextHandle = ASC->MakeEffectContext();
	RegenAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle RegenAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->RegenAttributes, 1.f, RegenAttributesContextHandle);

	ASC->ApplyGameplayEffectSpecToSelf(*RegenAttributesSpecHandle.Data.Get());
}

void UAuraAbilitySystemLibrary::InitializeDefaultAttributesFromAttributeSet(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, 
                                                                            UAttributeSet* AS)
{
	// 액터의 클래스 정보 가져오기
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr)
		return;
	
	UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(AS);
	if (!AuraAS)
		return;

	// Set by Caller 가져오기
	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	const AActor* SourceActor = ASC->GetAvatarActor();

	// 이펙트 컨텍스트 핸들 생성
	FGameplayEffectContextHandle EffectContextHandle = ASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(SourceActor);

	// 이펙트 적용을 위한 이펙트 스펙 핸들 생성
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->PrimaryAttributes_SetByCaller, 1.f, EffectContextHandle);

	float StrengthMag = AuraAS->GetStrength();
	float IntMag = AuraAS->GetIntelligence();
	float ResMag = AuraAS->GetResilience();
	float Vigor = AuraAS->GetVigor();
	
	// Set By Caller Magnitude 설정
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Strength, StrengthMag);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Intelligence, IntMag);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Resilience, ResMag);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Attributes_Primary_Vigor, Vigor);

	// 게임플레이 이펙트 적용
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 2차 속성 적용
	FGameplayEffectContextHandle SecondaryAttributesContextHandle = ASC->MakeEffectContext();
	SecondaryAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle SecondaryAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->SecondaryAttributes, 1.f, SecondaryAttributesContextHandle);
	ASC->ApplyGameplayEffectSpecToSelf(*SecondaryAttributesSpecHandle.Data.Get());

	// 바이탈 속성 적용
	FGameplayEffectContextHandle VitalAttributesContextHandle = ASC->MakeEffectContext();
	VitalAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle VitalAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->VitalAttributes_SetByCaller, 1.f, VitalAttributesContextHandle);
	
	float Health = AuraAS->GetHealth();
	float Mana = AuraAS->GetMana();
	
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(VitalAttributesSpecHandle, GameplayTags.Attributes_Vital_Health, Health);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(VitalAttributesSpecHandle, GameplayTags.Attributes_Vital_Mana, Mana);

	ASC->ApplyGameplayEffectSpecToSelf(*VitalAttributesSpecHandle.Data.Get());

	// 리젠 속성 적용
	FGameplayEffectContextHandle RegenAttributesContextHandle = ASC->MakeEffectContext();
	RegenAttributesContextHandle.AddSourceObject(SourceActor);
	
	const FGameplayEffectSpecHandle RegenAttributesSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo->RegenAttributes, 1.f, RegenAttributesContextHandle);

	ASC->ApplyGameplayEffectSpecToSelf(*RegenAttributesSpecHandle.Data.Get());
}

void UAuraAbilitySystemLibrary::GiveStartupAbilities(const UObject* WorldContextObject, UAbilitySystemComponent* ASC, ECharacterClass CharacterClass)
{
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr)
		return;

	for (TSubclassOf<UGameplayAbility> AbilityClass : CharacterClassInfo->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		FGameplayTagContainer ExcludeTags;
		ExcludeTags.AddTag(FGameplayTag::RequestGameplayTag("Abilities.Passive"));
		
		if (AbilitySpec.Ability.Get()->GetAssetTags().HasAny(ExcludeTags))
			continue;
		
		ASC->GiveAbility(AbilitySpec);
	}

	// 캐릭터 클래스에 맞는 정보 탐색
	const FCharacterClassDefaultInfo& DefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	for (auto& AbilityClass : DefaultInfo.StartupAbilities)
	{
		if (ASC->GetAvatarActor()->Implements<UCombatInterface>())
		{
			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, ICombatInterface::Execute_GetCharacterLevel(ASC->GetAvatarActor()));
			FGameplayTagContainer ExcludeTags;
			ExcludeTags.AddTag(FGameplayTag::RequestGameplayTag("Abilities.Passive"));
		
			if (AbilitySpec.Ability.Get()->GetAssetTags().HasAny(ExcludeTags))
				continue;
			
			ASC->GiveAbility(AbilitySpec);
		}
	}
}

void UAuraAbilitySystemLibrary::GiveStartupPassiveAbilities(const UObject* WorldContextObject,
	UAbilitySystemComponent* ASC, ECharacterClass CharacterClass)
{
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr)
		return;
	
	// 캐릭터 클래스에 맞는 정보 탐색
	const FCharacterClassDefaultInfo& DefaultInfo = CharacterClassInfo->GetClassDefaultInfo(CharacterClass);
	for (auto& AbilityClass : DefaultInfo.StartupPassiveAbilities)
	{
		if (ASC->GetAvatarActor()->Implements<UCombatInterface>())
		{
			FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, ICombatInterface::Execute_GetCharacterLevel(ASC->GetAvatarActor()));
			ASC->GiveAbilityAndActivateOnce(AbilitySpec);
		}
	}
}

int32 UAuraAbilitySystemLibrary::GetXPRewardForClassAndLevel(const UObject* WorldContextObject, ECharacterClass CharacterClass, int32 CharacterLevel)
{
	UCharacterClassInfo* CharacterClassInfo = GetCharacterClassInfo(WorldContextObject);
	if (CharacterClassInfo == nullptr)
		return 0;

	// 레벨에 맞는 경험치 값 가져오기
	float XP = CharacterClassInfo->GetClassDefaultInfo(CharacterClass).XPReward.GetValueAtLevel(CharacterLevel);

	return static_cast<int32>(XP);
}

FGameplayEffectContextHandle UAuraAbilitySystemLibrary::ApplyDamageEffect(const FDamageEffectParams& Params)
{
	UAuraAbilitySystemComponent* SourceASC = Cast<UAuraAbilitySystemComponent>(Params.SourceAbilitySystemComponent);
	UAuraAbilitySystemComponent* TargetASC = Cast<UAuraAbilitySystemComponent>(Params.TargetAbilitySystemComponent);

	if (SourceASC == TargetASC)
		return FGameplayEffectContextHandle();

	const FAuraGameplayTags& GameplayTags = FAuraGameplayTags::Get();

	AActor* SourceActor = SourceASC->GetAvatarActor();

	TSubclassOf<UGameplayEffect> EffectClass = Params.DamageGameplayEffectClass;

	// 이펙트 컨텍스트 핸들 생성
	FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(SourceActor);

	// 충격파 및 넉백 힘 설정
	SetDeathImpulse(EffectContextHandle, Params.DeathImpulse);
	SetKnockbackForce(EffectContextHandle, Params.KnockbackForce);

	// 범위 공격 변수 설정
	SetIsRadialDamage(EffectContextHandle, Params.bIsRadialDamage);
	SetRadialDamageInnerRadius(EffectContextHandle, Params.RadialDamageInnerRadius);
	SetRadialDamageOuterRadius(EffectContextHandle, Params.RadialDamageOuterRadius);
	SetRadialDamageOrigin(EffectContextHandle, Params.RadialDamageOrigin);

	// 이펙트 적용을 위한 이펙트 스펙 핸들 생성
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, Params.AbilityLevel, EffectContextHandle);
	if (!SpecHandle.IsValid())
		return FGameplayEffectContextHandle();
	
	// Set By Caller Magnitude 설정
	// 데미지 타입
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, Params.DamageType, Params.BaseDamage);

	// 마법 공격력 계수
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Damage_MagicAttackPowerCoefficient, Params.MagicPowerCoefficient);
	
	// 디버프
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Debuff_Chance, Params.DebuffChance);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Debuff_Damage, Params.DebuffDamage);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Debuff_Frequency, Params.DebuffFrequency);
	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, GameplayTags.Debuff_Duration, Params.DebuffDuration);
	
	// 타겟에게 데미지 이펙트 적용
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
	return EffectContextHandle;
}

TArray<FRotator> UAuraAbilitySystemLibrary::EvenlySpacedRotators(const FVector& Forward, const FVector& Axis, float Spread, int32 NumRotators)
{
	TArray<FRotator> Rotators;

	const FVector NormalizedAxis = Axis.GetSafeNormal();

	const FVector LeftSpread = Forward.RotateAngleAxis(-Spread / 2.f, NormalizedAxis);
	if (NumRotators > 1)
	{
		const float DeltaSpread = Spread / (NumRotators);
		for (int32 i = 0; i < NumRotators; i++)
		{
			// Z축 기준 DeltaSpread 만큼 회전
			const FVector Direction = LeftSpread.RotateAngleAxis(DeltaSpread * i, NormalizedAxis);
			Rotators.Add(Direction.Rotation());
		}
	}
	else
	{
		Rotators.Add(Forward.Rotation());
	}
	return Rotators;
}

TArray<FVector> UAuraAbilitySystemLibrary::EvenlyRotatedVectors(const FVector& Forward, const FVector& Axis, float Spread, int32 NumVectors)
{
	TArray<FVector> Vectors;

	const FVector LeftSpread = Forward.RotateAngleAxis(-Spread / 2.f, Axis);
	if (NumVectors > 1)
	{
		const float DeltaSpread = Spread / (NumVectors - 1);
		for (int32 i = 0; i < NumVectors; i++)
		{
			// Z축 기준으로 DeltaSpread 만큼 회전
			const FVector Direction = LeftSpread.RotateAngleAxis(DeltaSpread * i, FVector::UpVector);
			Vectors.Add(Direction);
		}
	}
	else
	{
		Vectors.Add(Forward);
	}
	return Vectors;
}

void UAuraAbilitySystemLibrary::ApplyMessageTagEffectToSelf(const FGameplayTag& Tag, AActor* AvatarActor, FText AppendText)
{
	// AAuraCharacter* AuraCharacter = Cast<AAuraCharacter>(AvatarActor);
	// if (AuraCharacter == nullptr)
	// 	return;
	//
	// UAbilitySystemComponent* ASC = AuraCharacter->GetAbilitySystemComponent();
	// if (ASC == nullptr)
	// 	return;
	//
	// // 기본 GE 클래스 생성 후 태그 부여
	// static TSubclassOf<UGameplayEffect> GETagGrantingClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/Blueprints/AbilitySystem/Aura/Effects/GE_GrandInfiniteTag.GE_GrandInfiniteTag_C")); 
	//
	// FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	// FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GETagGrantingClass, 1.f, EffectContext);
	//
	// if (SpecHandle.IsValid())
	// {
	// 	SpecHandle.Data.Get()->AddDynamicAssetTag(Tag);
	// 	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	// 	
	// 	FGameplayTagContainer CurrentOwnedTags;
	// 	ASC->GetOwnedGameplayTags(CurrentOwnedTags);
	// }
}

void UAuraAbilitySystemLibrary::AddMessageToActor(AActor* TargetActor, const FGameplayTag& MessageTag, FText AppendText, UTexture2D* Icon)
{
	UAuraGameInstance* GI = TargetActor->GetGameInstance<UAuraGameInstance>();
	if (!GI || !GI->MessageTable)
		return;
	
	if (AAuraPlayerController* PC = Cast<AAuraPlayerController>(Cast<APawn>(TargetActor)->GetController()))
	{
		PC->Client_CreateMessageWidget(MessageTag, AppendText, Icon);
	}
}

void UAuraAbilitySystemLibrary::RemoveMessageTagEffectToSelf(UAbilitySystemComponent* ASC, FGameplayTag MessageTag)
{
	ASC->RemoveActiveEffectsWithTags(FGameplayTagContainer(MessageTag));
	
	// if (auto* GI = Cast<UAuraGameInstance>(ASC->GetWorld()->GetGameInstance()))
	// {
	// 	if (FUIWidgetRow* FoundRow = GI->MessageTable->FindRow<FUIWidgetRow>(MessageTag.GetTagName(), "Found Message"))
	// 	{
	// 		if (auto AuraPS = Cast<AAuraPlayerState>(ASC->GetOwner()))
	// 		{
	// 			if (auto AuraPC = Cast<AAuraPlayerController>(AuraPS->GetPlayerController()))
	// 			{
	// 				if (auto AuraHUD = AuraPC->GetHUD<AAuraHUD>())
	// 				{
	// 					if (FoundRow->MessageWidget->GetDefaultObject<UAuraCenterDescriptionWidget>())
	// 					{
	// 						if (auto OverlayWidget = AuraHUD->GetOverlayWidget())
	// 						{
	// 							if (auto Center = OverlayWidget->WBP_CenterTutorialDescription)
	// 							{
	// 								Center->TextBlock->SetText(FText::GetEmpty());
	// 							}
	// 						}
	// 					}
	// 				}
	// 			}
	// 		}
	// 	}
	// }
}

TArray<FGameplayTag> UAuraAbilitySystemLibrary::GetAllActiveAbilityTagsFromAvatarActor(AActor* AvatarActor)
{
	TArray<FGameplayTag> AllActiveTags;
	TArray<FGameplayAbilitySpec> ActiveSpecs;
	
	if (auto* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(AvatarActor))
	{
		// 활성화된 어빌리티
		ActiveSpecs = ASC->GetActivatableAbilities();
	}
	
	for (FGameplayAbilitySpec& Spec : ActiveSpecs)
	{
		FGameplayTagContainer AbilityTagContainer = Spec.Ability->GetAssetTags();
		if (AbilityTagContainer.HasTag(FAuraGameplayTags::Get().Abilities_None))
			continue;

		for (auto Tag : AbilityTagContainer)
		{
			if (Tag.MatchesTag(FGameplayTag::RequestGameplayTag(TEXT("Abilities"))))
			{
				// 어빌리티 태그라면 추가
				AllActiveTags.Add(Tag);
			}
		}
	}
    
	return AllActiveTags;
}

int32 UAuraAbilitySystemLibrary::GetAbilityUpgradeStackCount(AActor* AvatarActor, const FGameplayTag& Tag)
{
	if (!Tag.IsValid() || !AvatarActor)
		return 0;

	if (APlayerController* PC = Cast<APlayerController>(Cast<APawn>(AvatarActor)->GetController()))
	{
		if (AAuraPlayerState* AuraPS = PC->GetPlayerState<AAuraPlayerState>())
		{
			return AuraPS->GetUpgradeTagCount(Tag);
		}
	}
	return 0;
}

int32 UAuraAbilitySystemLibrary::GetAbilityUpgradeStackCountByAuraPS(AAuraPlayerState* AuraPS, const FGameplayTag& Tag)
{
	if (!Tag.IsValid())
		return 0;

	if (AuraPS)
	{
		return AuraPS->GetUpgradeTagCount(Tag);
	}
	
	return 0;
}

bool UAuraAbilitySystemLibrary::IsThisMapTutorial(const UObject* WorldContextObject)
{
	const FString CurrentLevelName = WorldContextObject->GetWorld()->GetMapName();

	// 튜토리얼 레벨에서만 위젯 컨트롤러 생성
	if (CurrentLevelName.Contains(TEXT("Tutorial")))
		return true;

	return false;
}

APawn* UAuraAbilitySystemLibrary::SpawnGasActor(const UObject* WorldContextObject, TSubclassOf<APawn> SpawnClass, int32 Level, FTransform SpawnTransform, AActor* Owner)
{
	APawn* SpawningActor = WorldContextObject->GetWorld()->SpawnActorDeferred<APawn>
	(
		SpawnClass,
		SpawnTransform,
		Owner,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
		ESpawnActorScaleMethod::MultiplyWithRoot
	);

	if (!SpawningActor)
		return nullptr;
	
	if (AAuraEnemy* AuraEnemy = Cast<AAuraEnemy>(SpawningActor))
	{
		AuraEnemy->SetLevel(Level);
	}

	SpawningActor->FinishSpawning(SpawnTransform);

	return SpawningActor;
}

const FItemData UAuraAbilitySystemLibrary::GetItemDataByItemName(const UObject* WorldContextObject,
	const FName& ItemName)
{
	if (ItemName.IsNone())
		return FItemData();
	
	if (UAuraGameInstance* AuraGI = Cast<UAuraGameInstance>(UGameplayStatics::GetGameInstance(WorldContextObject)))
	{
		return *AuraGI->GetItemData(ItemName);
	}
	return FItemData();
}

UInventoryComponent* UAuraAbilitySystemLibrary::GetInventoryComponentByPlayerState(APlayerState* PlayerState)
{
	if (!IsValid(PlayerState))
	{
		UE_LOG(LogTemp, Warning, TEXT("GetInventoryComponentByPlayerState : PlayerState Is Not Valid!"));
		return nullptr;
	}
	
	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(PlayerState))
	{
		return AuraPS->GetInventoryComponent();
	}
	return nullptr;
}

UEquipmentComponent* UAuraAbilitySystemLibrary::GetEquipmentComponentByPlayerState(APlayerState* PlayerState)
{
	if (!IsValid(PlayerState))
	{
		UE_LOG(LogTemp, Warning, TEXT("GetEquipmentComponentByPlayerState : PlayerState Is Not Valid!"));
		return nullptr;
	}
	
	if (AAuraPlayerState* AuraPS = Cast<AAuraPlayerState>(PlayerState))
	{
		return AuraPS->GetEquipmentComponent();
	}
	return nullptr;
}

FString UAuraAbilitySystemLibrary::GetLocalIPAddress()
{
	bool bCanBindAll;
	TSharedPtr<FInternetAddr> LocalAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);

	if (LocalAddr.IsValid())
	{
		return LocalAddr->ToString(false);
	}

	return FString(TEXT("네트워크에 연결되지 않음"));
}

UCharacterClassInfo* UAuraAbilitySystemLibrary::GetCharacterClassInfo(const UObject* WorldContextObject)
{
	UAuraGameInstance* AuraGI = WorldContextObject->GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return nullptr;

	return AuraGI->CharacterClassInfo;
}

UAbilityInfo* UAuraAbilitySystemLibrary::GetAbilityInfo(const UObject* WorldContextObject)
{
	UAuraGameInstance* AuraGI = WorldContextObject->GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return nullptr;

	return AuraGI->AbilityInfo;
}

UAbilityUpgradeInfo* UAuraAbilitySystemLibrary::GetAbilityUpgradeInfo(const UObject* WorldContextObject)
{
	UAuraGameInstance* AuraGI = WorldContextObject->GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return nullptr;

	return AuraGI->AbilityUpgradeInfo;
}

FAuraAbilityUpgradeInfo UAuraAbilitySystemLibrary::GetAbilityUpgradeInfoForUpgradeTag(const UObject* WorldContextObject, const FGameplayTag& UpgradeTag)
{
	auto* AllUpgradeInfo = GetAbilityUpgradeInfo(WorldContextObject);
	if (!AllUpgradeInfo)
		return FAuraAbilityUpgradeInfo();

	if (!UpgradeTag.IsValid())
		return FAuraAbilityUpgradeInfo();

	return AllUpgradeInfo->GetUpgradeInfoForUpgradeTag(UpgradeTag);
}

ULootTiers* UAuraAbilitySystemLibrary::GetLootTiers(const UObject* WorldContextObject)
{
	UAuraGameInstance* AuraGI = WorldContextObject->GetWorld()->GetGameInstance<UAuraGameInstance>();
	if (AuraGI == nullptr)
		return nullptr;

	return AuraGI->LootTiers;
}

bool UAuraAbilitySystemLibrary::IsBlockedHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsBlockedHit();
	}
	return false;
}

bool UAuraAbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsCriticalHit();
	}
	return false;
}

bool UAuraAbilitySystemLibrary::IsSuccessfulDebuff(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->IsSuccessfulDebuff();
	}
	return false;
}

float UAuraAbilitySystemLibrary::GetDebuffDamage(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffDamage();
	}
	return 0.f;
}

float UAuraAbilitySystemLibrary::GetDebuffDuration(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffDuration();
	}
	return 0.f;
}

float UAuraAbilitySystemLibrary::GetDebuffFrequency(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDebuffFrequency();
	}
	return 0.f;
}

FGameplayTag UAuraAbilitySystemLibrary::GetDamageType(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		if (AuraEffectContext->GetDamagedType().IsValid())
		{
			return *AuraEffectContext->GetDamagedType();
		}
	}
	return FGameplayTag();
}

float UAuraAbilitySystemLibrary::GetMagicPowerCoefficient(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetMagicPowerCoefficient();
	}
	return 0.f;
}

FVector UAuraAbilitySystemLibrary::GetDeathImpulse(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetDeathImpulse();
	}
	return FVector::ZeroVector;
}

FVector UAuraAbilitySystemLibrary::GetKnockbackForce(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetKnockbackForce();
	}
	return FVector::ZeroVector;
}

bool UAuraAbilitySystemLibrary::IsRadialDamage(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetbIsRadialDamage();
	}
	return false;
}

float UAuraAbilitySystemLibrary::GetRadialDamageInnerRadius(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetRadialDamageInnerRadius();
	}
	return 0.f;
}

float UAuraAbilitySystemLibrary::GetRadialDamageOuterRadius(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetRadialDamageOuterRadius();
	}
	return 0.f;
}

FVector UAuraAbilitySystemLibrary::GetRadialDamageOrigin(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FAuraGameplayEffectContext* AuraEffectContext = static_cast<const FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return AuraEffectContext->GetRadialDamageOrigin();
	}
	return FVector::ZeroVector;
}

void UAuraAbilitySystemLibrary::GetLivePlayersWithinRadius(const UObject* WorldContextObject,
	TArray<AActor*>& OutOverlappingActors, const TArray<AActor*>& ActorsToIgnore, float Radius,
	const FVector& SphereOrigin)
{
	// 구형 충돌 계산
	FCollisionQueryParams SphereParams;
	SphereParams.AddIgnoredActors(ActorsToIgnore);

	// 겹침 판정
	TArray<FOverlapResult> Overlaps;
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		// SphereParams의 IgnoredActors를 제외한 겹침 판정 후 Overlaps에 채워 내보냄
		World->OverlapMultiByObjectType(Overlaps, SphereOrigin, FQuat::Identity, FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllDynamicObjects), FCollisionShape::MakeSphere(Radius), SphereParams);
		for (auto& Overlap : Overlaps)
		{
			// 겹친 대상의 아바타 액터를 취해 내보냄
			if (Overlap.GetActor()->Implements<UCombatInterface>() && !ICombatInterface::Execute_IsDead(Overlap.GetActor()))
			{
				OutOverlappingActors.AddUnique(ICombatInterface::Execute_GetAvatar(Overlap.GetActor()));
			}
		}
	}
}

void UAuraAbilitySystemLibrary::GetClosestTargets(int32 MaxTargets, const TArray<AActor*>& Actors, TArray<AActor*>& OutClosestTargets, const FVector& Origin)
{
	if (Actors.Num() <= MaxTargets)
	{
		OutClosestTargets = Actors;
		return;
	}

	TArray<AActor*> ActorsToCheck = Actors;
	int32 NumCheck = 0;

	while (NumCheck < MaxTargets)
	{
		if (ActorsToCheck.Num() == 0)
			break;

		double ClosestDistance = TNumericLimits<double>::Max();
		AActor* ClosestActor;
		for (AActor* Actor : ActorsToCheck)
		{
			const double Distance = (Origin - Actor->GetActorLocation()).Length();
			if (ClosestDistance > Distance)
			{
				ClosestDistance = Distance;
				ClosestActor = Actor;
			}
		}
		ActorsToCheck.Remove(ClosestActor);
		OutClosestTargets.AddUnique(ClosestActor);
		NumCheck++;
	}
}

bool UAuraAbilitySystemLibrary::IsNotFriend(AActor* FirstActor, AActor* SecondActor)
{
	if (!FirstActor || !SecondActor)
		return false;
	
	const bool bBothArePlayers = FirstActor->ActorHasTag(FName("Player")) && SecondActor->ActorHasTag(FName("Player"));
	const bool bBothAreEnemys = FirstActor->ActorHasTag(FName("Enemy")) && SecondActor->ActorHasTag(FName("Enemy"));

	// 플레이어 간, 몬스터 간 프렌들리 파이어 금지
	if (bBothArePlayers || bBothAreEnemys)
		return false;

	return true;
}

void UAuraAbilitySystemLibrary::SetIsBlockedHit(FGameplayEffectContextHandle& EffectContextHandle, bool bInIsBlockedHit)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsBlockedHit(bInIsBlockedHit);
	}
}

void UAuraAbilitySystemLibrary::SetIsCriticalHit(FGameplayEffectContextHandle& EffectContextHandle, bool bInIsCriticalHit)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsCriticalHit(bInIsCriticalHit);
	}
}

void UAuraAbilitySystemLibrary::SetIsSuccessfulDebuff(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, bool bInIsSuccessfulDebuff)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsSuccessfulDebuff(bInIsSuccessfulDebuff);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffDamage(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, float InDamage)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffDamage(InDamage);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffDuration(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, float InDuration)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffDuration(InDuration);
	}
}

void UAuraAbilitySystemLibrary::SetDebuffFrequency(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, float InFrequency)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDebuffFrequency(InFrequency);
	}
}

void UAuraAbilitySystemLibrary::SetDamageType(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, const FGameplayTag& InDamageType)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		const TSharedPtr<FGameplayTag> DamageType = MakeShared<FGameplayTag>(InDamageType);
		AuraEffectContext->SetDamageType(DamageType);
	}
}

void UAuraAbilitySystemLibrary::SetMagicPowerCoefficient(FGameplayEffectContextHandle& EffectContextHandle,
	float InCoefficient)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetMagicPowerCoefficient(InCoefficient);
	}
}

void UAuraAbilitySystemLibrary::SetDeathImpulse(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, const FVector& InImpulse)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetDeathImpulse(InImpulse);
	}
}

void UAuraAbilitySystemLibrary::SetKnockbackForce(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, const FVector& InForce)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetKnockbackForce(InForce);
	}
}

void UAuraAbilitySystemLibrary::SetIsRadialDamage(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, bool bInIsRadialDamage)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetIsRadialDamage(bInIsRadialDamage);
	}
}

void UAuraAbilitySystemLibrary::SetRadialDamageInnerRadius(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, float InRadialDamageInnerRadius)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetRadialDamageInnerRadius(InRadialDamageInnerRadius);
	}
}

void UAuraAbilitySystemLibrary::SetRadialDamageOuterRadius(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, float InRadialDamageOuterRadius)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetRadialDamageOuterRadius(InRadialDamageOuterRadius);
	}
}

void UAuraAbilitySystemLibrary::SetRadialDamageOrigin(UPARAM(ref)FGameplayEffectContextHandle& EffectContextHandle, FVector InRadialDamageOrigin)
{
	if (FAuraGameplayEffectContext* AuraEffectContext = static_cast<FAuraGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		AuraEffectContext->SetRadialDamageOrigin(InRadialDamageOrigin);
	}
}

void UAuraAbilitySystemLibrary::SetIsRadialDamageEffectParam(UPARAM(ref)FDamageEffectParams& DamageEffectParams, bool bIsRadial, FVector InOrigin, float InnerRadius, float OuterRadius)
{
	DamageEffectParams.bIsRadialDamage = bIsRadial;
	DamageEffectParams.RadialDamageOrigin = InOrigin;
	DamageEffectParams.RadialDamageInnerRadius = InnerRadius;
	DamageEffectParams.RadialDamageOuterRadius = OuterRadius;
}

void UAuraAbilitySystemLibrary::SetKnockbackDirection(UPARAM(ref)FDamageEffectParams& DamageEffectParams, FVector KnockbackDirection, float Magnitude)
{
	// 백터 정규화
	KnockbackDirection.Normalize();

	if (Magnitude == 0.f)
		DamageEffectParams.KnockbackForce = KnockbackDirection * DamageEffectParams.KnockbackForceMagnitude;
	else
		DamageEffectParams.KnockbackForce = KnockbackDirection * Magnitude;
}

void UAuraAbilitySystemLibrary::SetDeathImpulseDirection(UPARAM(ref)FDamageEffectParams& DamageEffectParams, FVector DeathImpulseDirection, float Magnitude)
{
	// 백터 정규화
	DeathImpulseDirection.Normalize();

	if (Magnitude == 0.f)
		DamageEffectParams.DeathImpulse = DeathImpulseDirection * DamageEffectParams.DeathImpulseMagnitude;
	else
		DamageEffectParams.DeathImpulse = DeathImpulseDirection * Magnitude;
}

void UAuraAbilitySystemLibrary::SetEffectParamsTargetAbilitySystemComponent(UPARAM(ref)FDamageEffectParams& DamageEffectParams, UAbilitySystemComponent* TargetASC)
{
	DamageEffectParams.TargetAbilitySystemComponent = TargetASC;
}

float UAuraAbilitySystemLibrary::GetAttributeValue(const UObject* WorldContextObject, const FGameplayTag& AttributeTag, bool bIsBaseValue)
{
	float Value = 0.f;
	if (APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		// PS, ASC, AS 가져오기
		AAuraPlayerState* AuraPS = PC->GetPlayerState<AAuraPlayerState>();
		if (AuraPS == nullptr)
			return Value;
		
		UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(AuraPS->GetAbilitySystemComponent());
		UAuraAttributeSet* AuraAttributeSet = Cast<UAuraAttributeSet>(AuraPS->GetAttributeSet());
	
		if (AuraASC && AuraAttributeSet)
		{
			if (AuraAttributeSet && AuraAttributeSet->TagsToAttributes.Contains(AttributeTag))
			{
				FGameplayAttribute Attribute = AuraAttributeSet->TagsToAttributes[AttributeTag]();

				if (bIsBaseValue)
				{
					Value = AuraASC->GetNumericAttributeBase(Attribute);
				}
				else
				{
					Value = AuraASC->GetNumericAttributeChecked(Attribute);
				}
			}
		}
	}

	// 몬스터라면
	if (const AAuraEnemy* Enemy = Cast<AAuraEnemy>(WorldContextObject))
	{
		UAuraAbilitySystemComponent* AuraASC = Cast<UAuraAbilitySystemComponent>(Enemy->GetAbilitySystemComponent());
		UAuraAttributeSet* AuraAS = Cast<UAuraAttributeSet>(Enemy->GetAttributeSet());

		if (AuraASC && AuraAS)
		{
			if (AuraAS->TagsToAttributes.Contains(AttributeTag))
			{
				FGameplayAttribute Attribute = AuraAS->TagsToAttributes[AttributeTag]();
				
				if (bIsBaseValue)
				{
					Value = AuraASC->GetNumericAttributeBase(Attribute);
				}
				else
				{
					Value = AuraASC->GetNumericAttributeChecked(Attribute);
				}
			}
		}
	}
		
	return Value;
}