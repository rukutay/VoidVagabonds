#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIShipController.generated.h"

class AShip;
class UPrimitiveComponent;
class AActor;
class USphereComponent;

UCLASS()
class VAGABONDSWORK_API AAIShipController : public AAIController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable)
    FVector GetFocusLocation();

    UFUNCTION(BlueprintCallable, Category="Rotation")
    void ApplyShipRotation(FVector TargetLocation);

    UFUNCTION(BlueprintCallable, Category="Unstuck")
    void SetCurrentObstacleComp(UPrimitiveComponent* ObstacleComp);

    UFUNCTION(BlueprintCallable, Category="Unstuck")
    UPrimitiveComponent* GetCurrentObstacleComp() const;

    UFUNCTION(BlueprintCallable, Category="Unstuck")
    FVector GetCurrentObstacleContactPoint() const;

    UFUNCTION(BlueprintCallable, Category="Unstuck")
    bool IsUnstucking() const;

    UFUNCTION(BlueprintCallable, Category="Navigation")
    bool IsInsideSafetyMargin() const { return bInsideSafetyMargin; }

    UFUNCTION(BlueprintCallable, Category="Navigation")
    FVector ComputeEscapeTarget(const FVector& ShipLocation, USphereComponent* ShipRadius) const;

private:
    void HandleStuckCheck();
    void HandleSafetyMarginCheck();
    bool UpdateInsideSafetyMargin(USphereComponent* ShipRadius);

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config")
    float StuckCheckInterval = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
    float NavSafetyMargin = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
    float SafetyExitHysteresisMultiplier = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance")
    float SafetyMarginCheckInterval = 0.15f;

    FTimerHandle StuckCheckTimerHandle;

    FTimerHandle SafetyMarginTimerHandle;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|State")
    TWeakObjectPtr<UPrimitiveComponent> CurrentObstacleComp;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance")
    TWeakObjectPtr<UPrimitiveComponent> CurrentNavObstacleComp;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance")
    TWeakObjectPtr<AActor> CurrentNavObstacleActor;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|State")
    bool bIsUnstucking = false;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance")
    bool bInsideSafetyMargin = false;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|State")
    float UnstuckEndTime = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|State")
    float LastUnstuckForceTime = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|Detection")
    float PrevDistanceToGoal = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|Detection")
    float StuckAccumulatedTime = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config")
    float MinStuckSpeed = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config")
    float StuckTimeThreshold = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config")
    float UnstuckForceStrength = 15000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config")
    float UnstuckDuration = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config")
    float UnstuckForceInterval = 0.1f;
};
