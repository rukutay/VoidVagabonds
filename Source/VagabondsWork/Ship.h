#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Ship.generated.h"

UENUM(BlueprintType)
enum class ERollAlignMode : uint8
{
    Default     UMETA(DisplayName="Default"),
    BackToTarget UMETA(DisplayName="Back"),   // ship +Up aligns to target direction
    BellyToTarget UMETA(DisplayName="Belly")  // ship -Up aligns to target direction
};

UENUM(BlueprintType)
enum class EShipPreset : uint8
{
    Fighter     UMETA(DisplayName="Fighter"),
    Interceptor UMETA(DisplayName="Interceptor"),
    Gunship     UMETA(DisplayName="Gunship"),
    Cruiser     UMETA(DisplayName="Cruiser"),
    Carrier     UMETA(DisplayName="Carrier"),
    Custom      UMETA(DisplayName="Custom")
};

USTRUCT(BlueprintType)
struct FShipTuningSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MaxPitchSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MaxYawSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float PitchAccelSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float YawAccelSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MaxRollSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float RollAccelSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MaxRollAngle = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    bool bUseTorquePD = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float TorqueKpPitch = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float TorqueKpYaw = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float TorqueKdPitch = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float TorqueKdYaw = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MaxTorquePitch = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MaxTorqueYaw = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MaxTorqueRoll = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float TorqueRollDamping = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float RollAlignKp = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float RollAlignKd = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MaxForwardForce = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float MinThrottle = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float ThrottleInterpSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning")
    float LateralDamping = 0.f;
};

class AAIShipController;
class AController;
class UShipNavComponent;
class UShipVitalityComponent;
class USpringArmComponent;
class UCameraComponent;
class UMarkerComponent;
class UNiagaraComponent;
class AActor;

UCLASS()
class VAGABONDSWORK_API AShip : public APawn
{
    GENERATED_BODY()

public:
    AShip();

    UFUNCTION(BlueprintCallable, Category="Ship|Camera")
    float GetCameraBoomLength() const;

    UFUNCTION(BlueprintCallable, Category="Ship|Camera")
    FTransform GetShipCameraTransform() const;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void UnPossessed() override;
    
    virtual void NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

    UFUNCTION()
    void HandleShipRadiusBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void HandlePatrolPointBeginOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult);

    UFUNCTION()
    void HandleShipRadiusEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    
    UFUNCTION(BlueprintCallable, Category="Ship|AI", meta=(ToolTip="Get current goal location for AI navigation."))
    FVector GetGoalLocation() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Navigation", meta=(ToolTip="Target actor used for AI navigation and orbiting."))
    AActor* TargetActor = nullptr;

    // ---------------- Orbit ----------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ToolTip="Enable orbiting around the target actor."))
    bool bOrbitTarget = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ToolTip="Automatically choose a stable orbit axis per ship/target."))
    bool bAutoOrbitAxis = true;

    // Prevent axis being too close to world up/down (avoids "almost horizontal" planes)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ClampMin="0.0", ClampMax="89.0", ToolTip="Minimum tilt away from world up/down for auto orbit axis."))
    float OrbitAxisMinTiltDeg = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ClampMin="0", ToolTip="Desired orbit radius in centimeters."))
    float EffectiveRange = 6000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ClampMin="0", ToolTip="Orbit angular speed in degrees per second."))
    float OrbitAngularSpeedDeg = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ToolTip="Manual orbit axis when auto-axis is disabled."))
    FVector OrbitAxis = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ToolTip="Orbit direction: clockwise when true."))
    bool bOrbitClockwise = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ClampMin="0.0", ToolTip="Extra tolerance before entering orbit ring."))
    float OrbitEnterToleranceMultiplier = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ToolTip="Seed for deterministic orbit start angle (0 = auto)."))
    int32 OrbitSeed = 0; // 0 = auto

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit|Debug", meta=(ToolTip="Draw orbit debug visuals."))
    bool bDebugOrbit = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Navigation|Debug", meta=(DisplayName="Debug Steering", ToolTip="Log steering target source and heading."))
    bool bDebugSteering = false;

    UFUNCTION(BlueprintCallable, Category="AI|Navigation", meta=(ToolTip="Get current target actor."))
    AActor* GetTargetActor() const { return TargetActor; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Preset", meta=(ToolTip="Preset for ship movement/rotation tuning."))
    EShipPreset ShipPreset = EShipPreset::Fighter;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Preset", meta=(ToolTip="Apply the selected preset at BeginPlay."))
    bool bApplyPresetOnBeginPlay = true;

    UFUNCTION(BlueprintCallable, Category="Ship|Tuning|Preset", meta=(ToolTip="Apply the current ship preset to movement/rotation values."))
    void ApplyShipPreset();

    UFUNCTION(BlueprintCallable, Category="Ship|Tuning|Controller", meta=(ToolTip="Apply ship controller tuning values to the AI controller."))
    void ApplyControllerTuning();

    // ---------------- Controller ----------------
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI", meta=(AllowPrivateAccess="true", ToolTip="AI controller driving ship navigation/rotation."))
    AAIShipController* ShipController = nullptr;

    // ---------------- Components ----------------
    UFUNCTION(BlueprintCallable, meta=(ToolTip="Get the ship's root physics mesh."))
    UStaticMeshComponent* GetShipBase() const;

    UFUNCTION(BlueprintCallable, Category="Ship|Components", meta=(ToolTip="Get the ship vitality component."))
    UShipVitalityComponent* GetVitality() const { return Vitality; }

    // ---------------- AI Controls ----------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ToolTip="Maximum pitch speed (deg/sec)."))
    float MaxPitchSpeed = 18.f;   // deg/sec

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ToolTip="Maximum yaw speed (deg/sec)."))
    float MaxYawSpeed = 18.f;    // deg/sec

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ToolTip="Pitch acceleration response."))
    float PitchAccelSpeed = 8.f;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ToolTip="Yaw acceleration response."))
    float YawAccelSpeed = 15.f;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ToolTip="Maximum roll speed (deg/sec)."))
    float MaxRollSpeed = 20.f;   // deg/sec

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ToolTip="Roll acceleration response."))
    float RollAccelSpeed = 15.f;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ToolTip="Maximum roll angle when banking (deg)."))
    float MaxRollAngle = 22.5f; // degrees

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Use torque-based PD control in the AI controller."))
    bool bUseTorquePD = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Torque PD pitch proportional gain."))
    float ControllerTorqueKpPitch = 18.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Torque PD yaw proportional gain."))
    float ControllerTorqueKpYaw = 22.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Torque PD pitch derivative gain."))
    float ControllerTorqueKdPitch = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Torque PD yaw derivative gain."))
    float ControllerTorqueKdYaw = 7.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Torque PD max pitch torque clamp."))
    float ControllerMaxTorquePitch = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Torque PD max yaw torque clamp."))
    float ControllerMaxTorqueYaw = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Torque PD max roll torque clamp."))
    float ControllerMaxTorqueRoll = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Controller", meta=(ToolTip="Torque PD roll damping gain."))
    float ControllerTorqueRollDamping = 0.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ToolTip="Roll alignment mode for AI rotation."))
    ERollAlignMode RollAlignMode = ERollAlignMode::Default;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|ManualControl", meta=(ToolTip="Allow roll alignment while in manual control."))
    bool bManualUseRollAlign = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|ManualControl", meta=(ClampMin="0.0", ToolTip="Shared yaw-to-roll bank scale for player and AI when roll align is disabled."))
    float YawBankScale = 1.0f;

    // Roll alignment gains (only used when RollAlignMode != Default)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ClampMin="0.0", ToolTip="Roll alignment proportional gain."))
    float RollAlignKp = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Rotation", meta=(ClampMin="0.0", ToolTip="Roll alignment derivative gain."))
    float RollAlignKd = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components", meta=(ToolTip="Root physics mesh for ship."))
    UStaticMeshComponent* ShipBase = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components", meta=(ToolTip="Camera boom for ship view."))
    USpringArmComponent* CameraBoom = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components", meta=(ToolTip="Ship camera."))
    UCameraComponent* ShipCamera = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Camera", meta=(ClampMin="0.0", ToolTip="Ship camera boom length."))
    float CameraBoomLength = 700.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components", meta=(ToolTip="Collision sphere used for radius checks."))
    USphereComponent* ShipRadius = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components", meta=(ToolTip="Internal radius scanner sphere."))
    USphereComponent* InternalScanerRadius = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Scanner", meta=(ToolTip="Actors inside scanner radius, sorted nearest first."))
    TArray<AActor*> WithinScaner;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Combat", meta=(ToolTip="Current combat opponents for this ship."))
    TArray<AActor*> CurrentOpponents;

    UFUNCTION(BlueprintCallable, Category="Ship|Combat", meta=(ToolTip="Add actor to current opponents list."))
    void AddOpponent(AActor* OpponentActor);

    UFUNCTION(BlueprintCallable, Category="Ship|Combat", meta=(ToolTip="Remove actor from current opponents list."))
    void RemoveOpponent(AActor* OpponentActor);

    UFUNCTION(BlueprintCallable, Category="Ship|Combat", meta=(ToolTip="Notify this ship that it has been attacked by the provided aggressor."))
    void NotifyIncomingAttack(AActor* AggressorActor);

    UFUNCTION(BlueprintCallable, Category="Ship|Combat", meta=(ToolTip="Remove invalid or destroyed actors from opponents list."))
    void PruneOpponents();

    UFUNCTION(BlueprintImplementableEvent, Category="Ship|Combat", meta=(ToolTip="Called when this ship is attacked."))
    void Attacked(AActor* AggressorActor);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI", meta=(ToolTip="Navigation component for avoidance and waypoints."))
    UShipNavComponent* ShipNav = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components", meta=(ToolTip="Ship health/shield component."))
    UShipVitalityComponent* Vitality = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components", meta=(ToolTip="Marker type component for map widget."))
    UMarkerComponent* MarkerComponent = nullptr;

    // ---------------- Movement ----------------
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Movement", meta=(ToolTip="Maximum forward force scalar."))
    float MaxForwardForce = 950.f;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Movement", meta=(ToolTip="Minimum throttle applied when steering."))
    float MinThrottle = 0.15f;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Movement", meta=(ToolTip="Throttle interpolation speed."))
    float ThrottleInterpSpeed = 10.f;

    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Tuning|Movement", meta=(ToolTip="Current throttle value applied to thrust."))
    float CurrentThrottle = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|State", meta=(ToolTip="True if ship is moving."))
    bool isMoving = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|State", meta=(ToolTip="True if current speed is greater than half of potential max speed."))
    bool isHalfSpeed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|State", meta=(ToolTip="True if a player is looking at this ship or controlling it."))
    bool IsPlayerLook = false;

    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning|Movement", meta=(ToolTip="Damping applied to lateral velocity."))
    float LateralDamping = 0.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Tuning", meta=(AllowPrivateAccess="true", ToolTip="Snapshot of ship tuning values."))
    FShipTuningSnapshot TuningSnapshot;

    // Soft Separation Settings
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Enable soft separation forces."))
    bool bEnableSoftSeparation = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Draw soft separation debug visuals."))
    bool bDebugSoftSeparation = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Extra separation margin beyond ship radius (cm)."))
    float SoftSeparationMarginCm = 30.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Base soft separation force strength."))
    float SoftSeparationStrength = 50000.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Maximum soft separation force clamp."))
    float SoftSeparationMaxForce = 90000.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Damping applied to separation response."))
    float SoftSeparationDamping = 2.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Smoothing response for separation force."))
    float SoftSeparationResponse = 6.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Maximum overlapping actors processed per tick."))
    int32 SoftSeparationMaxActors = 6;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation", meta=(ToolTip="Manual Control State."))
    bool bManualControl = false;

    
    UFUNCTION(BlueprintCallable, meta=(ToolTip="Apply forward steering force toward target location."))
    void ApplySteeringForce(FVector TargetLocation, float DeltaTime, bool bOverrideThrottle = false, float ThrottleOverride = 0.f);

    UFUNCTION(BlueprintCallable, Category="Ship|ManualControl", meta=(ToolTip="Set manual rotation input (pitch, yaw, roll)."))
    void SetManualRotationInput(float Pitch, float Yaw, float Roll);

    UFUNCTION(BlueprintCallable, Category="Ship|ManualControl", meta=(ToolTip="Set manual throttle input (-1 to 1)."))
    void SetManualThrottleInput(float Value);

    UFUNCTION(BlueprintCallable, Category="Ship|Rotation", meta=(ToolTip="Apply shared rotation servo toward target angular velocity (deg/sec)."))
    void ApplyRotationServo(const FVector& TargetAngVelLocal, float DeltaTime);

private:
    UFUNCTION()
    void HandleOpponentDestroyed(AActor* DestroyedActor);

    UFUNCTION()
    void UpdateWithinScaner();

    void UpdateNiagaraVelocity();

    bool EnsureShipController();
    void ApplyManualControl(float DeltaTime);
    void ApplyManualShipRotation(const FVector& TargetLocation, float DeltaTime);
    void ApplyManualTorqueControl(const FVector& TargetAngVelLocal, float DeltaTime);
    void CacheManualRotationTuning(const AAIShipController* PreviousController);
    FShipTuningSnapshot BuildTuningSnapshot() const;
    void ApplyTuningSnapshot(const FShipTuningSnapshot& Snapshot);
    void SyncControllerTuningFromShip();
    void SyncManualTuningFromShip();

    bool bLoggedMissingController = false;

    float ManualThrottleInput = 0.f;
    float ManualPitchInput = 0.f;
    float ManualYawInput = 0.f;
    float ManualRollInput = 0.f;
    float ManualAimDeadZoneDeg = 0.35f;
    bool bManualUseTorquePD = false;
    float ManualTorqueKpPitch = 0.f;
    float ManualTorqueKpYaw = 0.f;
    float ManualTorqueKdPitch = 0.f;
    float ManualTorqueKdYaw = 0.f;
    float ManualMaxTorquePitch = 0.f;
    float ManualMaxTorqueYaw = 0.f;
    float ManualTorqueRollDamping = 0.f;
    float ManualMaxTorqueRoll = 0.f;
    TWeakObjectPtr<AAIShipController> StoredAIController;

    float DebugMessageAccumulator = 0.f;
    float DebugSteeringAccumulator = 0.f;
    float SafetyStateTimeAccumulated = 0.f;
    float SafetyStatePrevGoalDistance = 0.f;
    float SafetyStateLogAccumulator = 0.f;
    float SafetyForceNavCooldown = 0.f;

    // Orbit runtime state (not replicated)
    bool bOrbitInitialized = false;
    float OrbitAngleRad = 0.0f;
    FVector OrbitBasisX = FVector::ForwardVector;
    FVector OrbitBasisY = FVector::RightVector;
    FVector LastOrbitAxis = FVector::UpVector;
    TWeakObjectPtr<AActor> LastOrbitTarget;

    FVector ComputeOrbitGoal(const FVector& Center, float DeltaTime);

    // Soft Separation Bookkeeping
    TArray<TWeakObjectPtr<UPrimitiveComponent>> SoftOverlapComps;
    FVector SmoothedSoftSeparationForce = FVector::ZeroVector;
    int32 SoftSeparationCleanupCounter = 0;
    
    // Performance optimization: reuse array to avoid allocations
    struct FOverlapDistanceInfo
    {
        TWeakObjectPtr<UPrimitiveComponent> Component;
        FVector ClosestPoint;
        float Distance;
        
        FOverlapDistanceInfo() : Component(nullptr), ClosestPoint(FVector::ZeroVector), Distance(0.0f) {}
        FOverlapDistanceInfo(TWeakObjectPtr<UPrimitiveComponent> InComponent, const FVector& InPoint, float InDistance)
            : Component(InComponent), ClosestPoint(InPoint), Distance(InDistance) {}
            
        bool operator<(const FOverlapDistanceInfo& Other) const
        {
            return Distance < Other.Distance; // Sort by closest first
        }
    };
    TArray<FOverlapDistanceInfo> OverlapDistancesCache;
    TArray<TWeakObjectPtr<UNiagaraComponent>> ThrusterNiagaraComponents;

    FTimerHandle WithinScanerUpdateTimer;

    // Soft Separation Force Application
    void ApplySoftSeparation(float DeltaTime);
};
