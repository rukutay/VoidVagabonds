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
	UPROPERTY(EditAnywhere, Category = "Navigation")
	float ReplanIntervalMin = 0.8f;

	UPROPERTY(EditAnywhere, Category = "Navigation")
	float ReplanIntervalMax = 1.6f;

	UPROPERTY(EditAnywhere, Category = "Navigation")
	float ReplanMinMovedDistanceMultiplier = 6.0f;

	UPROPERTY(EditAnywhere, Category = "Navigation")
	float ReplanMinGoalDeltaMultiplier = 4.0f;

	UPROPERTY(EditAnywhere, Category = "Navigation")
	float WaypointAcceptanceRadiusMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
	float NeighborQueryInterval = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
	float NeighborRadiusMultiplier = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
	int32 MaxNeighbors = 16;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
	float PredictionTimeMultiplier = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
	float AvoidWeight = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
	float GoalWeight = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
	float TempWaypointDistanceMultiplier = 4.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
	float SeparationMarginMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Navigation|Stuck")
	float StuckCheckInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Navigation|Stuck")
	float MinProgressMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Navigation|Stuck")
	int32 StuckThreshold = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid")
	float StaticAvoidMarginMultiplier = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid")
	float StaticAvoidShellMultiplier = 1.25f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid")
	float StaticAvoidTempDistMultiplier = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid")
	float StaticAvoidMinSpeedCmS = 50.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|StaticAvoid")
	float StaticAvoidRecheckInterval = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Navigation|Debug")
	bool bDrawNavPath = false;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TArray<FVector> GlobalWaypoints;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	int32 WaypointIndex = 0;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	FVector CurrentNavTarget = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance")
	TWeakObjectPtr<AActor> FocusActor;

	UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance")
	float CommitUntilTime = 0.0f;

	UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance")
	int32 SideSign = 0;

	UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance")
	bool bHasTempWaypoint = false;

	UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance")
	FVector TempWaypoint = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
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
