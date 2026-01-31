#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Ship.generated.h"

class AAIShipController;

UENUM(BlueprintType)
enum class EShipSteeringMode : uint8
{
    Default         UMETA(DisplayName="Default"),
    AggressiveSeek  UMETA(DisplayName="Aggressive Seek"),
};

UENUM(BlueprintType)
enum class EAvoidanceState : uint8
{
    None        UMETA(DisplayName="None"),
    AvoidCommit UMETA(DisplayName="Avoid Commit"),
    StuckEscape UMETA(DisplayName="Stuck Escape"),
};

USTRUCT(BlueprintType)
struct FShipSteeringState
{
    GENERATED_BODY()

    // Runtime-selected mode
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI")
    EShipSteeringMode Mode = EShipSteeringMode::Default;
};

USTRUCT(BlueprintType)
struct FObstacleDetectionSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float MinProbeDistanceMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float VelocityBlendMinSpeed = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float VelocityBlendForwardBias = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float VelocityBlendStrength = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float StaticRepathCooldown = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float ProbeLookAheadSeconds = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float ProbePaddingRadiusMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float ProbeSpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float ProbeLengthMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceClearanceMultiplier = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceTurnSlowdownAngle = 45.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceTurnThrottleScale = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceTurnBrakeStrength = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceTurnTorqueScale = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceTurnHoldSeconds = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceReachabilityProbeDistance = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceBehindOffsetMultiplier = 1.5f;
};

USTRUCT(BlueprintType)
struct FObstacleDebugSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance|Debug")
    bool bDrawProbe = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance|Debug")
    bool bDrawAvoidanceTarget = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance|Debug")
    bool bLogObstacleEvents = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance|Debug")
    float DebugDrawLifetime = 0.f;
};

UCLASS()
class VAGABONDSWORK_API AShip : public APawn
{
    GENERATED_BODY()

public:
    AShip();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // ---------------- AI / Mode ----------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI")
    FShipSteeringState SteeringState;

    UFUNCTION(BlueprintCallable, Category="Ship|AI")
    bool IsLocationVisible(const FVector& TargetLocation) const;

    UFUNCTION(BlueprintCallable, Category="Ship|AI")
    FVector GetGoalLocation() const;

    // ---------------- Controller ----------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI", meta=(AllowPrivateAccess="true"))
    AAIShipController* ShipController = nullptr;

    // ---------------- Avoidance ----------------
    UPROPERTY(EditAnywhere, Category="Ship|AI|Avoidance")
    float EvadeCheckInterval = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceTargetUpdateCooldown = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceStuckRadiusMultiplier = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceStuckCooldownIncrement = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceTargetInterpSpeed = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceCommitSeconds = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceCommitScoreThreshold = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    int32 AvoidanceSideSwitchWins = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceSideSwitchScoreBoost = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceStuckConfirmTime = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceMinSpeedWhileThrusting = 45.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceYawStuckAngle = 25.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceYawStuckRate = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceEscapeDuration = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidanceEscapeCooldown = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    float AvoidActorIgnoreDuration = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance")
    FObstacleDetectionSettings ObstacleDetectionSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Avoidance|Debug")
    FObstacleDebugSettings ObstacleDebugSettings;

    // Dynamic obstacles: local grid tuning
    UPROPERTY(EditAnywhere, Category="Ship|AI|Avoidance")
    int32 DynamicGridCells = 5;

    UPROPERTY(EditAnywhere, Category="Ship|AI|Avoidance")
    float DynamicGridGapMultiplier = 2.0f;

    // Static/physics: coarse A* tuning
    UPROPERTY(EditAnywhere, Category="Ship|AI|Avoidance")
    int32 AStarGridCells = 9;

    UPROPERTY(EditAnywhere, Category="Ship|AI|Avoidance")
    float AStarGapMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, Category="Ship|AI|Avoidance")
    float AStarRepathCooldown = 0.75f;

    UPROPERTY(EditAnywhere, Category="Ship|AI|Avoidance")
    int32 AStarMaxExpandedNodes = 1500;

    UPROPERTY()
    bool bHasAvoidanceTarget = false;

    UPROPERTY()
    FVector CurrentTargetLocation = FVector::ZeroVector;

    // Cached A* path (static/physics obstacles)
    UPROPERTY()
    TArray<FVector> CachedAvoidPath;

    UPROPERTY()
    int32 CachedAvoidPathIndex = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float AvoidanceThrottleScale = 1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float AvoidanceBrakeScale = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float AvoidanceTorqueScale = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float LastAvoidanceHitTime = -1.f;

    UPROPERTY()
    TWeakObjectPtr<AActor> CurrentAvoidActor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float AvoidActorIgnoreUntilTime = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float AvoidActorLastDistance = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    bool bAvoidanceRotationAssist = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    bool bUsingAvoidanceSequence = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float LastBigObstacleSequenceTime = -1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float LastAvoidanceTargetUpdateTime = -1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float CurrentAvoidanceTargetCooldown = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    bool bAvoidanceStuck = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    EAvoidanceState AvoidanceState = EAvoidanceState::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    FVector CommittedAvoidLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    bool bHasCommit = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float CommitExpireTime = -1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float CommitSideSign = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    int32 OppositeSideWinCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float CommitScore = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float StuckConfirmTimer = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float EscapeEndTime = -1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float EscapeCooldownUntil = -1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    FVector EscapeTarget = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    bool bEscapeReverse = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float LastDesiredYawDelta = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float LastYawSample = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float LastTargetDistance = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    FVector LastAvoidanceTargetLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    FVector AvoidanceStuckLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float LastAvoidanceStuckRampTime = -1.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Avoidance")
    float AvoidanceStuckStartTime = -1.f;

    float NextRepathTime = 0.f;

    bool bGoalVisible = false;

    FTimerHandle EvadeTimerHandle;

    UPROPERTY()
    TArray<FVector> AvoidanceSequence;

    UPROPERTY()
    int32 AvoidanceSequenceIndex = 0;

    bool bLastSweepStartPenetrating = false;

    UFUNCTION(BlueprintCallable)
    void EvadeThink();

    void UpdateAvoidanceRotationAssist(
        const FVector& AvoidanceTarget,
        const FVector& ForwardDir,
        const FVector& HitLocation,
        bool bHitObstacle
    );

    FVector CalculateVelocity() const;

    FVector CalculateProbeDirection(const FVector& ForwardDir, const FVector& Velocity) const;

    FVector AdjustAvoidanceTarget(
        const FVector& Candidate,
        const FVector& ObstacleLocation,
        const FVector& HitNormal,
        float Clearance
    ) const;

    void DebugDrawObstacleProbe(
        const FVector& Start,
        const FVector& End,
        bool bHit,
        const FHitResult& Hit
    ) const;

    void DebugDrawAvoidanceTarget(const FVector& Target, const FColor& Color) const;

    bool IsAvoidanceTargetReachable(
        const FVector& Start,
        const FVector& Target,
        float Radius
    ) const;

    FVector GetSideAvoidanceTarget(
        const FVector& Origin,
        const FVector& ForwardDir,
        const FVector& HitNormal,
        float Radius
    ) const;

    bool IsOverlappingBigObstacle() const;

    FVector EnforceAvoidanceMinDistance(
        const FVector& Start,
        const FVector& Target,
        const FVector& ForwardDir,
        float MinDistance
    ) const;

    FVector ComputeAvoidanceTargetFromStart(
        const FVector& StartLocation,
        const FVector& GoalLocation,
        const FVector& GridCenter,
        const FVector& HitLocation,
        const FVector& HitNormal,
        float Clearance,
        float Radius,
        const FVector& ForwardDir
    );

    // ---------------- Components ----------------
    UFUNCTION(BlueprintCallable)
    UStaticMeshComponent* GetShipBase() const;

    // ---------------- AI Controls ----------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxPitchSpeed = 6.f;   // deg/sec

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxYawSpeed = 18.f;    // deg/sec

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float PitchAccelSpeed = 6.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float YawAccelSpeed = 7.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxRollSpeed = 20.f;   // deg/sec

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float RollAccelSpeed = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxRollAngle = 22.5f; // degrees

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    UStaticMeshComponent* ShipBase = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    USphereComponent* ShipRadius = nullptr;

    // ---------------- Movement ----------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float MaxForwardForce = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float MinThrottle = 0.15f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float ThrottleInterpSpeed = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float BrakeInterpSpeed = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float SteeringTargetInterpSpeed = 3.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Movement")
    float CurrentThrottle = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Movement")
    float CurrentBrakeStrength = 0.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Movement")
    FVector SmoothedSteeringTarget = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Movement")
    float LateralDamping = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    bool bEnableAggressiveSeek = false;

    UFUNCTION(BlueprintCallable)
    void ApplySteeringForce(FVector TargetLocation, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category="Ship|AI|Movement")
    void ApplyAggressiveSeekForce(FVector TargetLocation, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category="Ship|AI|Movement")
    void ApplyUnstuckForce(float DeltaTime);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Unstuck")
    float UnstuckRadius = 300.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Unstuck")
    float UnstuckForceStrength = 300.f;

private:
    FVector PreviousShipLocation = FVector::ZeroVector;
};
