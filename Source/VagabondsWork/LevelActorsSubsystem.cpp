#include "LevelActorsSubsystem.h"

#include "EngineUtils.h"
#include "MarkerComponent.h"
#include "NavStaticBig.h"
#include "Ship.h"
#include "TimerManager.h"

void ULevelActorsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RefreshTrackedActors();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TrackedActorsRefreshTimer,
			this,
			&ULevelActorsSubsystem::RefreshTrackedActors,
			ActorListsRefreshInterval,
			true);
	}
}

void ULevelActorsSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TrackedActorsRefreshTimer);
	}

	Stations.Reset();
	Planets.Reset();
	Ships.Reset();

	Super::Deinitialize();
}

TArray<AActor*> ULevelActorsSubsystem::GetStationsAll() const
{
	TArray<AActor*> Result;
	Result.Reserve(Stations.Num());

	for (AActor* Station : Stations)
	{
		if (IsValid(Station))
		{
			Result.Add(Station);
		}
	}

	return Result;
}

TArray<ANavStaticBig*> ULevelActorsSubsystem::GetPlanetsAll() const
{
	TArray<ANavStaticBig*> Result;
	Result.Reserve(Planets.Num());

	for (ANavStaticBig* Planet : Planets)
	{
		if (IsValid(Planet))
		{
			Result.Add(Planet);
		}
	}

	return Result;
}

TArray<AShip*> ULevelActorsSubsystem::GetShipsAll() const
{
	TArray<AShip*> Result;
	Result.Reserve(Ships.Num());

	for (AShip* ShipActor : Ships)
	{
		if (IsValid(ShipActor))
		{
			Result.Add(ShipActor);
		}
	}

	return Result;
}

TArray<AActor*> ULevelActorsSubsystem::GetStationsOfFaction(EFaction Faction) const
{
	TArray<AActor*> Result;
	Result.Reserve(Stations.Num());

	for (AActor* Station : Stations)
	{
		if (!IsValid(Station))
		{
			continue;
		}

		const UMarkerComponent* Marker = Station->FindComponentByClass<UMarkerComponent>();
		if (IsValid(Marker) && Marker->Faction == Faction)
		{
			Result.Add(Station);
		}
	}

	return Result;
}

TArray<ANavStaticBig*> ULevelActorsSubsystem::GetPlanetsOfFaction(EFaction Faction) const
{
	TArray<ANavStaticBig*> Result;
	Result.Reserve(Planets.Num());

	for (ANavStaticBig* Planet : Planets)
	{
		if (!IsValid(Planet))
		{
			continue;
		}

		const UMarkerComponent* Marker = Planet->FindComponentByClass<UMarkerComponent>();
		if (IsValid(Marker) && Marker->Faction == Faction)
		{
			Result.Add(Planet);
		}
	}

	return Result;
}

TArray<AShip*> ULevelActorsSubsystem::GetShipsOfFaction(EFaction Faction) const
{
	TArray<AShip*> Result;
	Result.Reserve(Ships.Num());

	for (AShip* ShipActor : Ships)
	{
		if (!IsValid(ShipActor))
		{
			continue;
		}

		const UMarkerComponent* Marker = ShipActor->FindComponentByClass<UMarkerComponent>();
		if (IsValid(Marker) && Marker->Faction == Faction)
		{
			Result.Add(ShipActor);
		}
	}

	return Result;
}

void ULevelActorsSubsystem::RefreshTrackedActors()
{
	Stations.Reset();
	Planets.Reset();
	Ships.Reset();

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
				Planets.Add(Planet);
			}
		}
		else if (Marker->MarkerType == EMarkerType::Ship)
		{
			if (AShip* ShipActor = Cast<AShip>(Actor))
			{
				Ships.Add(ShipActor);
			}
		}
		else if (Marker->MarkerType == EMarkerType::Station)
		{
			Stations.Add(Actor);
		}
	}
}