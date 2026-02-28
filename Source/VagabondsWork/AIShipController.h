#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIShipController.generated.h"

class AShip;
class ANavStaticBig;
class UPrimitiveComponent;
class AActor;
class USphereComponent;

UENUM(BlueprintType)
enum class EActionMode : uint8
{
    Idle      UMETA(DisplayName="Idle"),
    Moving    UMETA(DisplayName="Moving"),
    Following UMETA(DisplayName="Following"),
    Patroling UMETA(DisplayName="Patroling"),
    Fight     UMETA(DisplayName="Fight"),
    Flee      UMETA(DisplayName="Flee")
};

UCLASS()
class VAGABONDSWORK_API AAIShipController : public AAIController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="Set current AI action mode."))
    void SetActionMode(EActionMode InActionMode) { ActionMode = InActionMode; }

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="Get current AI action mode."))
    EActionMode GetActionMode() const { return ActionMode; }

    UFUNCTION(BlueprintCallable, Category="Navigation|Action", meta=(ToolTip="Reset current AI action mode to Idle."))
    void ResetAction();

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

    UFUNCTION(BlueprintCallable, Category="Navigation", meta=(ToolTip="Temporarily suppress safety margin checks (seconds)."))
    void SuppressSafetyMargin(float DurationSeconds);

    UFUNCTION(BlueprintCallable, Category="Navigation|Movement", meta=(ToolTip="Returns whether movement is currently allowed for the controlled ship."))
    bool IsMovementAllowed() const { return bMovementAllowed; }

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="Build and store an effective patrol route from a random subset of provided NavStaticBig actors."))
    TArray<ANavStaticBig*> CreatePatrolRoute(const TArray<ANavStaticBig*>& NavStaticActors);

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="Start patrol progression using generated route and per-point delay."))
    void StartPatrol(const TArray<ANavStaticBig*>& NavStaticActors, float InPointDelaySeconds);

    UFUNCTION(BlueprintCallable, Category="Navigation|Follow", meta=(ToolTip="Start following a target ship."))
    void StartFollowing(AShip* TargetShip);

    UFUNCTION(BlueprintCallable, Category="Navigation|Movement", meta=(ToolTip="Move to target actor until within effective range."))
    void MoveToTarget(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category="Navigation|Movement", meta=(ToolTip="Move to target actor and finish on overlap or half effective range."))
    void GoToActor(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category="Navigation|Movement", meta=(ToolTip="True while GoToActor arrival conditions are being tracked."))
    bool IsGoToActorActive() const { return bGoToActorActive; }

    UFUNCTION(BlueprintCallable, Category="Navigation|Fight", meta=(ToolTip="Enter fight mode and assign target to controlled ship and external modules."))
    void Fight(AActor* TargetActor);

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="Get current patrol route head target."))
    ANavStaticBig* GetCurrentPatrolPoint() const;

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="Handle overlap for patrol point completion checks."))
    void HandlePatrolPointOverlap(UPrimitiveComponent* OtherComp, AActor* OtherActor);

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="Stop patrol progression and optionally clear route."))
    void StopPatrol(bool bClearRoute);

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="True when patrol route is currently in progress (including delay pause)."))
    bool IsPatrolActive() const { return bPatrolActive && PatrolRoute.Num() > 0; }

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="True when patrol route is currently in progress (including delay pause)."))
    bool IsPatrolInProgress() const { return bPatrolActive && PatrolRoute.Num() > 0; }

    UFUNCTION(BlueprintCallable, Category="Navigation|Patrol", meta=(ToolTip="True when patrol route has no points left or is not running."))
    bool IsPatrolFinished() const { return !IsPatrolInProgress(); }

    UFUNCTION(BlueprintCallable, Category="Navigation", meta=(ToolTip="Get safety margin suppression duration."))
    float GetSafetyMarginSuppressDuration() const { return SafetyMarginSuppressDuration; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="True when using torque-based rotation."))
    bool UsesTorquePD() const { return bUseTorquePD; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Torque PD pitch proportional gain."))
    float GetTorqueKpPitch() const { return TorqueKpPitch; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Torque PD yaw proportional gain."))
    float GetTorqueKpYaw() const { return TorqueKpYaw; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Torque PD pitch derivative gain."))
    float GetTorqueKdPitch() const { return TorqueKdPitch; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Torque PD yaw derivative gain."))
    float GetTorqueKdYaw() const { return TorqueKdYaw; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Torque PD max pitch torque clamp."))
    float GetMaxTorquePitch() const { return MaxTorquePitch; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Torque PD max yaw torque clamp."))
    float GetMaxTorqueYaw() const { return MaxTorqueYaw; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Torque PD roll damping gain."))
    float GetTorqueRollDamping() const { return TorqueRollDamping; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Torque PD max roll torque clamp."))
    float GetMaxTorqueRoll() const { return MaxTorqueRoll; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Aim error dead zone in degrees."))
    float GetAimDeadZoneDeg() const { return AimDeadZoneDeg; }

    UFUNCTION(BlueprintCallable, Category="Rotation", meta=(ToolTip="Apply torque PD tuning values."))
    void SetTorqueTuning(
        bool bEnableTorquePD,
        float InTorqueKpPitch,
        float InTorqueKpYaw,
        float InTorqueKdPitch,
        float InTorqueKdYaw,
        float InMaxTorquePitch,
        float InMaxTorqueYaw,
        float InMaxTorqueRoll,
        float InTorqueRollDamping);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation|Patrol", meta=(ToolTip="Current high-level AI action mode."))
    EActionMode ActionMode = EActionMode::Idle;

private:
    void HandleStuckCheck();
    void HandleSafetyMarginCheck();
    void SetExternalModulesTarget(AActor* TargetActor);
    void ClearFightTargetState();

    UFUNCTION()
    void HandleFightTargetDestroyed(AActor* DestroyedActor);

    bool UpdateInsideSafetyMargin(USphereComponent* ShipRadius);
    void ResumePatrolAfterDelay();
    void PruneInvalidPatrolHead();

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Fight", meta=(ToolTip="Currently tracked fight target."))
    TWeakObjectPtr<AActor> CurrentFightTarget;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Movement", meta=(ToolTip="Currently tracked GoToActor target."))
    TWeakObjectPtr<AActor> CurrentGoToActorTarget;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Movement", meta=(ToolTip="True while GoToActor arrival conditions are active."))
    bool bGoToActorActive = false;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Config", meta=(ToolTip="Interval (seconds) between stuck checks."))
    float StuckCheckInterval = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Extra safety margin added beyond ship radius."))
    float NavSafetyMargin = 0.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Multiplier for exit hysteresis beyond enter margin."))
    float SafetyExitHysteresisMultiplier = 0.5f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Interval (seconds) between safety margin checks."))
    float SafetyMarginCheckInterval = 0.15f;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Avoidance", meta=(ToolTip="Seconds to suppress safety margin checks after forcing Nav fallback."))
    float SafetyMarginSuppressDuration = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation|Movement", meta=(AllowPrivateAccess="true", ToolTip="Enable/disable AI ship movement."))
    bool bMovementAllowed = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Navigation|Patrol", meta=(AllowPrivateAccess="true", ToolTip="Stored ordered patrol route points in world space."))
    TArray<TObjectPtr<ANavStaticBig>> PatrolRoute;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Patrol", meta=(ToolTip="True when patrol progression is active."))
    bool bPatrolActive = false;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Patrol", meta=(ToolTip="True while patrol progression waits post-point delay."))
    bool bPatrolPauseActive = false;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Patrol", meta=(ToolTip="Delay after reaching patrol point before continuing."))
    float PatrolPointDelaySeconds = 0.0f;

    FTimerHandle PatrolResumeTimerHandle;

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Patrol", meta=(ToolTip="Cached current patrol route head when valid."))
    TWeakObjectPtr<ANavStaticBig> CurrentPatrolTarget;

    UPROPERTY(EditDefaultsOnly, Category = "Navigation|Debug", meta=(ToolTip="Draw safety margin debug points."))
    bool bDebugSafetyMargin = false;

    UPROPERTY(EditDefaultsOnly, Category = "Unstuck|Debug", meta=(ToolTip="Enable unstuck debug logging."))
    bool bDebugUnstuck = false;

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

    UPROPERTY(VisibleAnywhere, Category = "Navigation|Avoidance", meta=(ToolTip="World time until safety margin checks are suppressed."))
    float SafetyMarginSuppressUntilTime = 0.0f;

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
    float MaxTorquePitch = 1.f; // N*cm (clamp)

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Max yaw torque clamp (N*cm)."))
    float MaxTorqueYaw = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Roll damping gain when not roll-aligning."))
    float TorqueRollDamping = 0.2f; // roll angular velocity damping (rad/s)

    UPROPERTY(EditDefaultsOnly, Category="Rotation|TorquePD", meta=(ToolTip="Max roll torque clamp (N*cm)."))
    float MaxTorqueRoll = 1.f;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|RollLevel", meta=(ToolTip="Enable passive roll leveling when moving forward."))
    bool bEnableForwardRollLevel = true;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|RollLevel", meta=(ToolTip="Forward speed threshold (cm/s) to enable roll leveling."))
    float ForwardRollLevelSpeedThreshold = 50.f;

    UPROPERTY(EditDefaultsOnly, Category="Rotation|RollLevel", meta=(ToolTip="Multiplier for roll level PD gains."))
    float ForwardRollLevelGainScale = 1.0f;

    UPROPERTY(VisibleAnywhere, Category="Rotation|Debug", meta=(ToolTip="Accumulator for roll alignment debug logging."))
    float RollAlignDebugAccumulator = 0.0f;
};
