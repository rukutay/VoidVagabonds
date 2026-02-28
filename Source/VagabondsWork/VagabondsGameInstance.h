// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "MarkerComponent.h"
#include "VagabondsGameInstance.generated.h"

UENUM(BlueprintType)
enum class EActorFilterKind : uint8
{
	Faction,
	MarkerType
};

class ANavStaticBig;
class AShip;

USTRUCT(BlueprintType)
struct FNavObstacleSphereProxy
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	float BaseRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	float InflatedRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TArray<FVector> Anchors;
};

UCLASS()
class VAGABONDSWORK_API UVagabondsGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void OnStart() override;

	const TArray<FNavObstacleSphereProxy>& GetStaticNavObstacles() const;
	bool GetClosestObstacleToSegment(const FVector& A, const FVector& B, int32& OutIndex) const;
	bool IsSegmentClearOfStaticObstacles(const FVector& A, const FVector& B, int32* OutFirstHitIndex = nullptr) const;
	const FNavObstacleSphereProxy* FindObstacleByActor(AActor* InActor) const;
	void SetRuntimeNavObstacleActors(const TArray<AActor*>& Actors);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	TArray<FVector> FindGlobalPathAnchors(const FVector& Start, const FVector& Goal) const;

	UFUNCTION(BlueprintCallable, Category = "Navigation|Debug")
	bool DebugTestSegmentAgainstStaticObstacles(const FVector& A, const FVector& B);

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<AActor*> GetActorsByFilter(const TArray<AActor*>& Actors, EActorFilterKind FilterKind, uint8 EnumValue) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float NavSafetyMarginCm = 200.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Actors")
	TArray<TObjectPtr<ANavStaticBig>> AllPlanets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Actors")
	TArray<TObjectPtr<AShip>> AllShips;

	UPROPERTY(EditDefaultsOnly, Category = "World|Actors")
	float ActorListsRefreshInterval = 0.05f;

private:
	void InitializeNavObstacles();
	void RefreshMovingStaticObstacles();
	void RefreshCombinedNavObstacles();
	void RefreshTrackedActors();

	FTimerHandle TrackedActorsRefreshTimer;
	FTimerHandle MovingStaticRefreshTimer;
};
