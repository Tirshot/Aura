// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"

/**
 * 
 */
struct FAuraGameplayTags
{
private:
	// 생성자를 private으로 만들어서 외부에서 임의로 인스턴스를 생성하는 것을 방지
	FAuraGameplayTags() {}

	// 복사 생성자 및 대입 연산자 제거
	FAuraGameplayTags(const FAuraGameplayTags&) = delete;
	FAuraGameplayTags& operator=(const FAuraGameplayTags&) = delete;

public:
	static const FAuraGameplayTags& Get() { return GameplayTags; }
	static void InitailizeNativeGameplayTags();
	void InitializeAbilitiesTagsArray();
	void InitializeAttributesTagsArray();

	FGameplayTag Attributes_Primary_Strength;
	FGameplayTag Attributes_Primary_Intelligence;
	FGameplayTag Attributes_Primary_Resilience;
	FGameplayTag Attributes_Primary_Vigor;

	FGameplayTag Attributes_Secondary_MagicAttackPower;
	FGameplayTag Attributes_Secondary_Armor;
	FGameplayTag Attributes_Secondary_ArmorPenetration;
	FGameplayTag Attributes_Secondary_BlockChance;
	FGameplayTag Attributes_Secondary_CriticalHitChance;
	FGameplayTag Attributes_Secondary_CriticalHitDamage;
	FGameplayTag Attributes_Secondary_CriticalHitResistance;
	FGameplayTag Attributes_Secondary_HealthRegeneration;
	FGameplayTag Attributes_Secondary_ManaRegeneration;
	FGameplayTag Attributes_Secondary_MaxHealth;
	FGameplayTag Attributes_Secondary_MaxMana;
	FGameplayTag Attributes_Secondary_MovementSpeed;

	FGameplayTag Attributes_Vital_Health;
	FGameplayTag Attributes_Vital_Mana;

	FGameplayTag Attributes_Meta_IncomingXP;

	// 입력 태그
	FGameplayTag InputTag_LMB;
	FGameplayTag InputTag_RMB;

	FGameplayTag InputTag_1;
	FGameplayTag InputTag_2;
	FGameplayTag InputTag_3;
	FGameplayTag InputTag_4;

	// 패시브 태그
	FGameplayTag InputTag_Passive_1;
	FGameplayTag InputTag_Passive_2;

	// 데미지 태그
	FGameplayTag Damage;
	FGameplayTag Damage_Fire;
	FGameplayTag Damage_Lightning;
	FGameplayTag Damage_Arcane;
	FGameplayTag Damage_Physical;
	FGameplayTag Damage_MagicAttackPowerCoefficient;

	// 속성 저항
	FGameplayTag Attributes_Resistance_Fire;
	FGameplayTag Attributes_Resistance_Arcane;
	FGameplayTag Attributes_Resistance_Lightning;
	FGameplayTag Attributes_Resistance_Physical;

	// 디버프
	FGameplayTag Debuff_Burn;
	FGameplayTag Debuff_Stun;
	FGameplayTag Debuff_Arcane;
	FGameplayTag Debuff_Physical;

	FGameplayTag Debuff_Chance;
	FGameplayTag Debuff_Damage;
	FGameplayTag Debuff_Duration;
	FGameplayTag Debuff_Frequency;

	// 게임플레이 어빌리티
	FGameplayTag Abilities_None;

	FGameplayTag Abilities_Attack;
	FGameplayTag Abilities_Summon;
	FGameplayTag Abilities_Fire_FireBolt;
	FGameplayTag Abilities_Fire_FireBlast;
	FGameplayTag Abilities_Fire_Firenado;
	FGameplayTag Abilities_Lightning_Electrocute;
	FGameplayTag Abilities_Lightning_Teleport;
	FGameplayTag Abilities_Lightning_SpawnElectroSphere;
	FGameplayTag Abilities_Arcane_ArcaneShards;
	FGameplayTag Abilities_Arcane_MindControl;
	FGameplayTag Abilities_Arcane_ArcaneOrbit;
	FGameplayTag Abilities_Arcane_ArcaneArea;
	FGameplayTag Abilities_HitReact;
	
	// 패시브 어빌리티
	FGameplayTag Abilities_Passive;
	FGameplayTag Abilities_Passive_HaloOfProtection;
	FGameplayTag Abilities_Passive_LifeSiphon;
	FGameplayTag Abilities_Passive_ManaSiphon;

	// 어빌리티 상태
	FGameplayTag Abilities_Status_Locked;
	FGameplayTag Abilities_Status_Eligible;
	FGameplayTag Abilities_Status_Unlocked;
	FGameplayTag Abilities_Status_Equipped;
	FGameplayTag Abilities_Status_Activated; // 실행 중인 액티브 스펠

	// 어빌리티 태그
	FGameplayTag Abilities_Type_Offensive;
	FGameplayTag Abilities_Type_Passive;
	FGameplayTag Abilities_Type_None;

	// 어빌리티 업그레이드 태그(로그라이크)
	FGameplayTag Upgrades_Fire_Increase10PercentDamage;
	FGameplayTag Upgrades_Fire_FireBolt_Temp;
	FGameplayTag Upgrades_Fire_FireBolt_IncreaseNum;
	
	FGameplayTag Upgrades_Fire_FireBlast_Temp;
	FGameplayTag Upgrades_Fire_FireBlast_IncreaseNum;
	
	FGameplayTag Upgrades_Fire_FireNado_IncreaseRange;
	
	FGameplayTag Upgrades_Arcane_Increase10PercentDamage;
	FGameplayTag Upgrades_Arcane_ArcaneShards_Temp;
	FGameplayTag Upgrades_Arcane_ArcaneShards_IncreaseNum;
	FGameplayTag Upgrades_Arcane_ArcaneShards_FirstLargeShard;
	
	FGameplayTag Upgrades_Arcane_ArcaneOrbit_IncreaseRange;
	FGameplayTag Upgrades_Arcane_ArcaneOrbit_IncreaseNum;
	
	FGameplayTag Upgrades_Arcane_ArcaneArea_IncreaseRange;
	FGameplayTag Upgrades_Arcane_ArcaneArea_IncreaseSize;
	
	FGameplayTag Upgrades_Lightning_Increase10PercentDamage;
	FGameplayTag Upgrades_Lightning_Electrocute_AdditionalTarget;
	
	FGameplayTag Upgrades_Lightning_Teleport_Temp;
	FGameplayTag Upgrades_Lightning_Teleport_DecreaseCoolDown;
	FGameplayTag Upgrades_Lightning_Teleport_ReturnToInitLocation;
	
	FGameplayTag Upgrades_Lightning_SpawnElectroSphere_IncreaseTraceRange;
	FGameplayTag Upgrades_Lightning_SpawnElectroSphere_DecreaseMovementSpeed;
	FGameplayTag Upgrades_Lightning_SpawnElectroSphere_HomingNearestTarget;

	// 쿨다운 태그
	FGameplayTag Cooldown_Fire_FireBolt;

	// TaggedMontage Struct
	FGameplayTag CombatSocket_Weapon;
	FGameplayTag CombatSocket_RightHand;
	FGameplayTag CombatSocket_LeftHand;
	FGameplayTag CombatSocket_Tail;

	FGameplayTag Montage_Attack_1;
	FGameplayTag Montage_Attack_2;
	FGameplayTag Montage_Attack_3;
	FGameplayTag Montage_Attack_4;

	TMap<FGameplayTag, FGameplayTag> DamageTypesToResistances;
	TMap<FGameplayTag, FGameplayTag> DamageTypesToDebuff;
	TArray<FGameplayTag> GameplayAbilitiesTags;
	TArray<FGameplayTag> AttributesTags;

	// 피격 반응
	FGameplayTag Effects_HitReact;

	// 입력 방지
	FGameplayTag Player_Block_InputPressed;
	FGameplayTag Player_Block_InputHeld;
	FGameplayTag Player_Block_InputReleased;
	FGameplayTag Player_Block_CursorTrace;
	FGameplayTag Player_Abilities_WaitForExecute;

	// 게임플레이 큐
	FGameplayTag GameplayCue_FireBlast;

	// 초기화 게임플레이 이펙트
	FGameplayTag Init_Attributes;

private:
	static FAuraGameplayTags GameplayTags;
};
