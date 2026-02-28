// Fill out your copyright notice in the Description page of Project Settings.

#include "VagabondsGameInstance.h"
#include "NavStaticBig.h"
#include "Ship.h"
#include "EngineUtils.h"

void UVagabondsGameInstance::OnStart()
{
	Super::OnStart();

	RefreshTrackedActors();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TrackedActorsRefreshTimer,
			this,
			&UVagabondsGameInstance::RefreshTrackedActors,
			ActorListsRefreshInterval,
			true);
	}
}

TArray<AActor*> UVagabondsGameInstance::GetActorsByFilter(const TArray<AActor*>& Actors, EActorFilterKind FilterKind, uint8 EnumValue) const
{
	TArray<AActor*> Result;
	Result.Reserve(Actors.Num());

	auto TryAddActorIfMatch = [&](AActor* Actor)
	{
		if (!IsValid(Actor))
		{
			return;
		}

		const UMarkerComponent* Marker = Actor->FindComponentByClass<UMarkerComponent>();
		if (!IsValid(Marker))
		{
			return;
		}

		switch (FilterKind)
		{
		case EActorFilterKind::Faction:
			if (Marker->Faction == static_cast<EFaction>(EnumValue))
			{
				Result.Add(Actor);
			}
			break;

		case EActorFilterKind::MarkerType:
			if (Marker->MarkerType == static_cast<EMarkerType>(EnumValue))
			{
				Result.Add(Actor);
			}
			break;

		default:
			break;
		}
	};

	for (AActor* Actor : Actors)
	{
		TryAddActorIfMatch(Actor);
	}

	return Result;
}

void UVagabondsGameInstance::RefreshTrackedActors()
{
	AllPlanets.Reset();
	AllShips.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}

		const UMarkerComponent* Marker = Actor->FindComponentByClass<UMarkerComponent>();
		if (!IsValid(Marker))
		{
			continue;
		}

		if (Marker->MarkerType == EMarkerType::Planet)
		{
			if (ANavStaticBig* Planet = Cast<ANavStaticBig>(Actor))
			{
				AllPlanets.Add(Planet);
			}
		}
		else if (Marker->MarkerType == EMarkerType::Ship)
		{
			if (AShip* ShipActor = Cast<AShip>(Actor))
			{
				AllShips.Add(ShipActor);
			}
		}
	}
}

