// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NavigationSubsystem.h"
#include "VagabondsWorkGameMode.generated.h"

UCLASS(minimalapi)
class AVagabondsWorkGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AVagabondsWorkGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	const TArray<FNavObstacleSphereProxy>& GetStaticNavObstacles() const;
	bool IsSegmentClearOfStaticObstacles(const FVector& A, const FVector& B, int32* OutFirstHitIndex = nullptr) const;
	const FNavObstacleSphereProxy* FindObstacleByActor(AActor* InActor) const;
	void SetRuntimeNavObstacleActors(const TArray<AActor*>& Actors);
	TArray<FVector> FindGlobalPathAnchors(const FVector& Start, const FVector& Goal) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float NavSafetyMarginCm = 15000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float NavAnchorShellMultiplier = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	int32 NavAnchorsPerObstacle = 24;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float DefaultShipRadiusCm = 150.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	bool bNavDebugDrawStatic = false;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|MovingStatic")
	float StaticObstacleRefreshInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|MovingStatic")
	FName MovingStaticTag = "NavStaticMoving";

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TArray<FNavObstacleSphereProxy> StaticNavObstacles;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TArray<FNavObstacleSphereProxy> RuntimeNavObstacles;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TArray<FNavObstacleSphereProxy> CombinedNavObstacles;

private:
	void InitializeNavObstacles();
	void RefreshMovingStaticObstacles();
	void RefreshCombinedNavObstacles();

	FTimerHandle MovingStaticRefreshTimer;
};



