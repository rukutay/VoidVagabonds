#include "MarkerComponent.h"
#include "Engine/GameInstance.h"

UMarkerComponent::UMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	Faction = EFaction::VoidVagabonds;
}

void UMarkerComponent::BeginPlay()
{
	Super::BeginPlay();
}