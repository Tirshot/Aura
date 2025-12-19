// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/AuraDragDropOperation.h"

#include "AbilitySystem/AuraAbilitySystemLibrary.h"
#include "Interaction/PlayerInterface.h"
#include "UI/ViewModel/MVVM_Inventory.h"

void UAuraDragDropOperation::DragCancelled_Implementation(const FPointerEvent& PointerEvent)
{
	Super::DragCancelled_Implementation(PointerEvent);
	
	if (PointerEvent.IsKeyEvent())
		return;
	
	if (Inventory)
		Inventory->Server_RemoveItemToWorld(OriginalIndex);
	else
	{
		//
		// if (GetOwner()->Implements<UPlayerInterface>())
		// {
		// 	EquipmentComponent = IPlayerInterface::Execute_GetEquipmentComponent(GetOwner());
		// }
	}
}
