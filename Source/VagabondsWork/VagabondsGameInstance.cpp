// Fill out your copyright notice in the Description page of Project Settings.

#include "VagabondsGameInstance.h"
#include "LevelActorsSubsystem.h"

void UVagabondsGameInstance::OnStart()
{
	Super::OnStart();
	GetSubsystem<ULevelActorsSubsystem>();
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
