// Fill out your copyright notice in the Description page of Project Settings.

#include "MapWidget.h"
#include "LevelBoundaries.h"
#include "NavStaticBig.h"
#include "Ship.h"
#include "Sun.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

void UMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RefreshMarkers();

	UWorld* World = GetWorld();
	if (World)
	{
		const float SafeInterval = FMath::Max(RefreshIntervalSec, 0.1f);
		World->GetTimerManager().SetTimer(RefreshTimerHandle, this, &UMapWidget::RefreshMarkers, SafeInterval, true);
	}
}

void UMapWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

	Super::NativeDestruct();
}

void UMapWidget::RefreshMarkers()
{
	TArray<FMapMarkerData> NewMarkers;
	CollectPlayerShipMarkers(NewMarkers);
	CollectSunMarker(NewMarkers);
	CollectNavStaticBigMarkers(NewMarkers);
	Markers = MoveTemp(NewMarkers);
}

float UMapWidget::GetMapRadiusCm() const
{
	if (const ALevelBoundaries* Boundaries = FindLevelBoundaries())
	{
		return Boundaries->GetBoundaryRadius();
	}

	return 0.0f;
}

FVector2D UMapWidget::MapToPixelTopLeftFromWidget(const FVector2D& NormalizedPosition, const UWidget* MapWidget, float DotSizePx, const FVector2D& FallbackMapSizePx) const
{
	if (!MapWidget)
	{
		return FVector2D::ZeroVector;
	}

	FVector2D MapSize = MapWidget->GetCachedGeometry().GetLocalSize();
	if (MapSize.IsNearlyZero())
	{
		MapSize = MapWidget->GetDesiredSize();
		if (MapSize.IsNearlyZero())
		{
			MapSize = FallbackMapSizePx;
		}
	}

	const FVector2D HalfSize = MapSize * 0.5f;
	const FVector2D PixelCenter = (NormalizedPosition * HalfSize) + HalfSize;
	const FVector2D DotOffset(DotSizePx * 0.5f, DotSizePx * 0.5f);
	return PixelCenter - DotOffset;
}

void UMapWidget::CollectPlayerShipMarkers(TArray<FMapMarkerData>& OutMarkers) const
{
	UWorld* World = GetWorld();
	const float MapRadius = GetMapRadiusCm();
	if (!World || MapRadius <= 0.0f)
	{
		return;
	}

	const APawn* PossessedPawn = GetOwningPlayerPawn();

	for (TActorIterator<AShip> It(World); It; ++It)
	{
		AShip* Ship = *It;
		if (!Ship || !Ship->ActorHasTag(TEXT("player")))
		{
			continue;
		}

		FMapMarkerData Marker;
		Marker.MapPosition = ProjectWorldToMap(Ship->GetActorLocation(), MapRadius);
		Marker.Color = (Ship == PossessedPawn) ? Settings.PossessedShipColor : Settings.TaggedPlayerShipColor;
		Marker.SizePx = Settings.DefaultMarkerSizePx;
		Marker.SourceActor = Ship;
		OutMarkers.Add(Marker);
	}
}

void UMapWidget::CollectSunMarker(TArray<FMapMarkerData>& OutMarkers) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ASun* SunActor = Cast<ASun>(UGameplayStatics::GetActorOfClass(World, ASun::StaticClass()));
	if (!SunActor)
	{
		return;
	}

	FMapMarkerData Marker;
	Marker.MapPosition = FVector2D::ZeroVector;
	Marker.Color = Settings.SunColor;
	Marker.SizePx = Settings.DefaultMarkerSizePx;
	Marker.SourceActor = SunActor;
	OutMarkers.Add(Marker);
}

void UMapWidget::CollectNavStaticBigMarkers(TArray<FMapMarkerData>& OutMarkers) const
{
	UWorld* World = GetWorld();
	const float MapRadius = GetMapRadiusCm();
	if (!World || MapRadius <= 0.0f)
	{
		return;
	}

	for (TActorIterator<ANavStaticBig> It(World); It; ++It)
	{
		ANavStaticBig* NavActor = *It;
		if (!NavActor)
		{
			continue;
		}

		FMapMarkerData Marker;
		Marker.MapPosition = ProjectWorldToMap(NavActor->GetActorLocation(), MapRadius);
		Marker.Color = Settings.NavStaticBigColor;
		Marker.SizePx = Settings.DefaultMarkerSizePx;
		Marker.SourceActor = NavActor;
		OutMarkers.Add(Marker);
	}
}

FVector2D UMapWidget::ProjectWorldToMap(const FVector& WorldLocation, float MapRadiusCm) const
{
	if (MapRadiusCm <= 0.0f)
	{
		return FVector2D::ZeroVector;
	}

	float X = WorldLocation.X;
	float Y = WorldLocation.Y;

	if (bSwapMapXY)
	{
		Swap(X, Y);
	}

	float NormalizedX = FMath::Clamp(X / MapRadiusCm, -1.0f, 1.0f);
	float NormalizedY = FMath::Clamp(Y / MapRadiusCm, -1.0f, 1.0f);

	if (bInvertMapY)
	{
		NormalizedY *= -1.0f;
	}

	return FVector2D(NormalizedX, NormalizedY);
}

ALevelBoundaries* UMapWidget::FindLevelBoundaries() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return Cast<ALevelBoundaries>(UGameplayStatics::GetActorOfClass(World, ALevelBoundaries::StaticClass()));
}