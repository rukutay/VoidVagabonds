// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipNavComponent.generated.h"

UENUM()
enum class ETempWaypointReason : uint8
{
	None,
	Dynamic,
	Static
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VAGABONDSWORK_API UShipNavComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UShipNavComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void TickNav(float DeltaTime, const FVector& GoalLocation, float ShipRadiusCm, bool bMovingGoal);
    FVector GetNavTarget(const FVector& GoalLocation) const;

protected:
    UPROPERTY(EditAnywhere, Category = "Navigation", meta=(ToolTip="Minimum time between replans (seconds)."))
	float ReplanIntervalMin = 0.8f;

    UPROPERTY(EditAnywhere, Category = "Navigation", meta=(ToolTip="Maximum time between replans (seconds)."))
	float ReplanIntervalMax = 1.6f;

    UPROPERTY(EditAnywhere, Category = "Navigation", meta=(ToolTip="Replan if moved beyond radius * multiplier."))
	float ReplanMinMovedDistanceMultiplier = 6.0f;

    UPROPERTY(EditAnywhere, Category = "Navigation", meta=(ToolTip="Replan if goal moved beyond radius * multiplier."))
	float ReplanMinGoalDeltaMultiplier = 4.0f;

    UPROPERTY(EditAnywhere, Category = "Navigation", meta=(ToolTip="Waypoint acceptance radius multiplier."))
	float WaypointAcceptanceRadiusMultiplier = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Interval between neighbor queries (seconds)."))
	float NeighborQueryInterval = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Idle backoff multiplier for neighbor queries."))
	float NeighborQueryIdleBackoffMultiplier = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Speed threshold for idle backoff (cm/s)."))
	float NeighborQueryIdleSpeedCmS = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Neighbor query radius multiplier."))
	float NeighborRadiusMultiplier = 8.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Maximum neighbors processed per query."))
	int32 MaxNeighbors = 16;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Prediction time multiplier for dynamic avoidance."))
	float PredictionTimeMultiplier = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Weight for avoidance steering."))
	float AvoidWeight = 1.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Weight for goal-seeking steering."))
	float GoalWeight = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Temp waypoint distance multiplier."))
	float TempWaypointDistanceMultiplier = 4.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Extra separation margin for dynamic avoidance."))
	float SeparationMarginMultiplier = 0.5f;

    UPROPERTY(EditAnywhere, Category = "Navigation|Stuck", meta=(ToolTip="Interval between stuck checks (seconds)."))
	float StuckCheckInterval = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Navigation|Stuck", meta=(ToolTip="Minimum progress required to avoid stuck counter."))
	float MinProgressMultiplier = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Navigation|Stuck", meta=(ToolTip="Consecutive failed checks before stuck recovery."))
	int32 StuckThreshold = 6;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid", meta=(ToolTip="Extra radius margin for static obstacles."))
	float StaticAvoidMarginMultiplier = 0.75f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid", meta=(ToolTip="Shell multiplier for tangent avoidance."))
	float StaticAvoidShellMultiplier = 1.25f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid", meta=(ToolTip="Temp waypoint distance for static avoid (multiplier)."))
	float StaticAvoidTempDistMultiplier = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid", meta=(ToolTip="Minimum speed before using velocity for static avoid."))
	float StaticAvoidMinSpeedCmS = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid", meta=(ToolTip="Interval between static rechecks (seconds)."))
	float StaticAvoidRecheckInterval = 0.05f;

    UPROPERTY(EditAnywhere, Category = "Navigation|Debug", meta=(ToolTip="Draw navigation debug visuals."))
	bool bDrawNavPath = false;

    UPROPERTY(EditAnywhere, Category = "Navigation|Debug", meta=(ToolTip="Log navigation target/avoidance decisions."))
	bool bDebugNav = false;

    UPROPERTY(VisibleAnywhere, Category = "Navigation", meta=(ToolTip="Current global waypoint list."))
	TArray<FVector> GlobalWaypoints;

    UPROPERTY(VisibleAnywhere, Category = "Navigation", meta=(ToolTip="Current waypoint index in global path."))
	int32 WaypointIndex = 0;

    UPROPERTY(VisibleAnywhere, Category = "Navigation", meta=(ToolTip="Current navigation target (temp or waypoint)."))
	FVector CurrentNavTarget = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="Focused dynamic obstacle actor."))
	TWeakObjectPtr<AActor> FocusActor;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="Commit window end time for avoidance side."))
	float CommitUntilTime = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="Avoidance side sign (-1/1)."))
	int32 SideSign = 0;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="True when using a temporary avoidance waypoint."))
	bool bHasTempWaypoint = false;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="Temporary avoidance waypoint location."))
	FVector TempWaypoint = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, Category = "Navigation", meta=(ToolTip="Reason for current temporary waypoint."))
	ETempWaypointReason TempReason = ETempWaypointReason::None;

	float NextStuckCheckTime = 0.0f;
	float LastDistanceToTarget = 0.0f;
	int32 StuckCounter = 0;
	float AvoidWeightBoostUntilTime = 0.0f;
	float AvoidWeightBoostStartValue = 0.0f;
	float AvoidWeightBaseValue = 0.0f;
	float NextStaticRecheckTime = 0.0f;
	int32 FocusStaticObstacleIndex = INDEX_NONE;

	float NextNeighborQueryTime = 0.0f;
	TArray<TWeakObjectPtr<AActor>> CachedNeighbors;

	float NextReplanTime = 0.0f;
	FVector LastReplanShipPos = FVector::ZeroVector;
	FVector LastReplanGoal = FVector::ZeroVector;
	float StaticBlockedAccumTime = 0.0f;
};
