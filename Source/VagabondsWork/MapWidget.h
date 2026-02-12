// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MapWidget.generated.h"

class ALevelBoundaries;
class ANavStaticBig;

USTRUCT(BlueprintType)
struct FMapMarkerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FVector2D MapPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	float SizePx = 6.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TWeakObjectPtr<AActor> SourceActor;
};

USTRUCT(BlueprintType)
struct FMapSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	float DefaultMarkerSizePx = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FLinearColor PossessedShipColor = FLinearColor(0.1f, 0.9f, 0.1f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FLinearColor TaggedPlayerShipColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FLinearColor NavStaticBigColor = FLinearColor(0.6f, 0.2f, 0.8f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map")
	FLinearColor SunColor = FLinearColor(1.0f, 0.8f, 0.2f, 1.0f);
};

UCLASS()
class VAGABONDSWORK_API UMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Map")
	void RefreshMarkers();

	UFUNCTION(BlueprintPure, Category = "Map")
	float GetMapRadiusCm() const;

	UFUNCTION(BlueprintPure, Category = "Map")
	FVector2D MapToPixelTopLeftFromWidget(const FVector2D& NormalizedPosition, const UWidget* MapWidget, float DotSizePx, const FVector2D& FallbackMapSizePx) const;

	UPROPERTY(BlueprintReadOnly, Category = "Map")
	TArray<FMapMarkerData> Markers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ShowOnlyInnerProperties))
	FMapSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Debug")
	bool bInvertMapY = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map|Debug")
	bool bSwapMapXY = false;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void CollectPlayerShipMarkers(TArray<FMapMarkerData>& OutMarkers) const;
	void CollectSunMarker(TArray<FMapMarkerData>& OutMarkers) const;
	void CollectNavStaticBigMarkers(TArray<FMapMarkerData>& OutMarkers) const;
	FVector2D ProjectWorldToMap(const FVector& WorldLocation, float MapRadiusCm) const;
	ALevelBoundaries* FindLevelBoundaries() const;

	FTimerHandle RefreshTimerHandle;

	UPROPERTY(EditAnywhere, Category = "Map")
	float RefreshIntervalSec = 0.25f;
};