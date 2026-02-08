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

    UFUNCTION(BlueprintCallable, meta=(ToolTip="Get focus location (target actor if set, otherwise pawn location)."))
    FVector GetFocusLocation();

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Apply rotation control toward a target world location."))
    void ApplyShipRotation(FVector TargetLocation);

    UFUNCTION(BlueprintCallable, Category="Unstuck", meta=(ToolTip="Set the current obstacle component used for stuck/safety checks."))
    void SetCurrentObstacleComp(UPrimitiveComponent* ObstacleComp);

    UFUNCTION(BlueprintCallable, Category="Unstuck", meta=(ToolTip="Get the current obstacle component used for stuck/safety checks."))
    UPrimitiveComponent* GetCurrentObstacleComp() const;

    UFUNCTION(BlueprintCallable, Category="Unstuck", meta=(ToolTip="Get closest contact point on current obstacle to the pawn."))
    FVector GetCurrentObstacleContactPoint() const;

    UFUNCTION(BlueprintCallable, Category="Unstuck", meta=(ToolTip="Return true if the controller is applying unstuck behavior."))
    bool IsUnstucking() const;

    UFUNCTION(BlueprintCallable, Category="Navigation", meta=(ToolTip="True when pawn is inside the configured safety margin."))
    bool IsInsideSafetyMargin() const { return bInsideSafetyMargin; }

    UFUNCTION(BlueprintCallable, Category="Navigation", meta=(ToolTip="Compute a temporary escape target when inside the safety margin."))
    FVector ComputeEscapeTarget(const FVector& ShipLocation, USphereComponent* ShipRadius) const;

private:
    void HandleStuckCheck();
    void HandleSafetyMarginCheck();
    bool UpdateInsideSafetyMargin(USphereComponent* ShipRadius);

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config", meta=(ToolTip="Interval (seconds) between stuck checks."))
    float StuckCheckInterval = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Extra safety margin added beyond ship radius."))
    float NavSafetyMargin = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Multiplier for exit hysteresis beyond enter margin."))
    float SafetyExitHysteresisMultiplier = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Interval (seconds) between safety margin checks."))
    float SafetyMarginCheckInterval = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Debug", meta=(ToolTip="Draw safety margin debug points."))
    bool bDebugSafetyMargin = false;

    FTimerHandle StuckCheckTimerHandle;

    FTimerHandle SafetyMarginTimerHandle;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|State", meta=(ToolTip="Current obstacle component used for stuck logic."))
    TWeakObjectPtr<UPrimitiveComponent> CurrentObstacleComp;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="Obstacle component tracked for safety margin checks."))
    TWeakObjectPtr<UPrimitiveComponent> CurrentNavObstacleComp;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="Obstacle actor tracked for safety margin checks."))
    TWeakObjectPtr<AActor> CurrentNavObstacleActor;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|State", meta=(ToolTip="True when currently applying unstuck forces."))
    bool bIsUnstucking = false;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="True when inside the safety margin."))
    bool bInsideSafetyMargin = false;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|State", meta=(ToolTip="World time when unstuck behavior ends."))
    float UnstuckEndTime = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|State", meta=(ToolTip="Last time an unstuck force was applied."))
    float LastUnstuckForceTime = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|Detection", meta=(ToolTip="Previous distance to goal for stuck detection."))
    float PrevDistanceToGoal = 0.0f;

    UPROPERTY(VisibleAnywhere, Category = "Unstuck|Detection", meta=(ToolTip="Accumulated time spent in a stuck state."))
    float StuckAccumulatedTime = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config", meta=(ToolTip="Minimum speed considered not stuck (cm/s)."))
    float MinStuckSpeed = 50.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config", meta=(ToolTip="Time (seconds) below min speed before unstuck triggers."))
    float StuckTimeThreshold = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config", meta=(ToolTip="Strength of unstuck push force."))
    float UnstuckForceStrength = 15000.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config", meta=(ToolTip="Duration (seconds) for unstuck behavior."))
    float UnstuckDuration = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config", meta=(ToolTip="Minimum interval between unstuck force impulses."))
    float UnstuckForceInterval = 0.1f;

private:
    UPROPERTY(VisibleAnywhere, Category="Rotation|Smoothing", meta=(ToolTip="Smoothed pitch rate (deg/sec) used for control."))
    float FilteredPitchRate = 0.f;

    UPROPERTY(VisibleAnywhere, Category="Rotation|Smoothing", meta=(ToolTip="Smoothed yaw rate (deg/sec) used for control."))
    float FilteredYawRate = 0.f;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|Smoothing", meta=(ToolTip="Max pitch rate change (deg/sec^2)."))
    float MaxPitchRateChange = 120.f; // deg/sec^2 (rate slew)

    UPROPERTY(EditDefaultsOnly, Category="Rotation|Smoothing", meta=(ToolTip="Max yaw rate change (deg/sec^2)."))
    float MaxYawRateChange = 180.f;   // deg/sec^2

    UPROPERTY(EditDefaultsOnly, Category="Rotation|Smoothing", meta=(ToolTip="Aim error dead zone in degrees."))
    float AimDeadZoneDeg = 0.35f;     // error under this treated as 0

    UPROPERTY(EditDefaultsOnly, Category="Rotation|Smoothing", meta=(ToolTip="Use torque-based PD instead of angular velocity."))
    bool bUseTorquePD = false;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Pitch proportional gain (rad -> ang accel)."))
    float TorqueKpPitch = 18.f; // (rad -> ang accel)

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Yaw proportional gain (rad -> ang accel)."))
    float TorqueKpYaw = 22.f;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Pitch derivative gain (rad/s damping)."))
    float TorqueKdPitch = 6.f;  // (rad/s damping)

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Yaw derivative gain (rad/s damping)."))
    float TorqueKdYaw = 7.f;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Max pitch torque clamp (N*cm)."))
    float MaxTorquePitch = 8.0e7f; // N*cm (clamp)

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Max yaw torque clamp (N*cm)."))
    float MaxTorqueYaw = 1.0e8f;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Roll damping gain when not roll-aligning."))
    float TorqueRollDamping = 4.f; // roll angular velocity damping (rad/s)

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Max roll torque clamp (N*cm)."))
    float MaxTorqueRoll = 3.0e7f;
};
