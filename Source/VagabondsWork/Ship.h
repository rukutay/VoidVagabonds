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

class AAIShipController;
class AController;
class UShipNavComponent;
class UShipVitalityComponent;

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
    void HandleShipRadiusEndOverlap(
        UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);

    // NAVIGATION_TODO_REMOVE
    UFUNCTION(BlueprintCallable, Category="Ship|AI")
    FVector GetGoalLocation() const;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="AI|Navigation")
    AActor* TargetActor = nullptr;

    // ---------------- Orbit ----------------
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="AI|Orbit")
    bool bOrbitTarget = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit")
    bool bAutoOrbitAxis = true;

    // Prevent axis being too close to world up/down (avoids "almost horizontal" planes)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ClampMin="0.0", ClampMax="89.0"))
    float OrbitAxisMinTiltDeg = 15.0f;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="AI|Orbit", meta=(ClampMin="0"))
    float EffectiveRange = 6000.0f; // cm

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ClampMin="0"))
    float OrbitAngularSpeedDeg = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit")
    FVector OrbitAxis = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit")
    bool bOrbitClockwise = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit", meta=(ClampMin="0.0"))
    float OrbitEnterToleranceMultiplier = 0.15f;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="AI|Orbit")
    int32 OrbitSeed = 0; // 0 = auto

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Orbit|Debug")
    bool bDebugOrbit = false;

    UFUNCTION(BlueprintCallable, Category="AI|Navigation")
    AActor* GetTargetActor() const { return TargetActor; }

    // ---------------- Controller ----------------
    // NAVIGATION_TODO_REMOVE
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI", meta=(AllowPrivateAccess="true"))
    AAIShipController* ShipController = nullptr;

    // ---------------- Components ----------------
    UFUNCTION(BlueprintCallable)
    UStaticMeshComponent* GetShipBase() const;

    UFUNCTION(BlueprintCallable, Category="Ship|Components")
    UShipVitalityComponent* GetVitality() const { return Vitality; }

    // ---------------- AI Controls ----------------
    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxPitchSpeed = 6.f;   // deg/sec

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxYawSpeed = 18.f;    // deg/sec

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float PitchAccelSpeed = 6.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float YawAccelSpeed = 7.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxRollSpeed = 20.f;   // deg/sec

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float RollAccelSpeed = 10.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxRollAngle = 22.5f; // degrees

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Rotation")
    ERollAlignMode RollAlignMode = ERollAlignMode::Default;

    // Roll alignment gains (only used when RollAlignMode != Default)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Rotation", meta=(ClampMin="0.0"))
    float RollAlignKp = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI|Rotation", meta=(ClampMin="0.0"))
    float RollAlignKd = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    UStaticMeshComponent* ShipBase = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    USphereComponent* ShipRadius = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI")
    UShipNavComponent* ShipNav = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    UShipVitalityComponent* Vitality = nullptr;

    // ---------------- Movement ----------------
    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float MaxForwardForce = 500.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float MinThrottle = 0.15f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float ThrottleInterpSpeed = 2.5f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Movement")
    float CurrentThrottle = 0.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float LateralDamping = 2.5f;

    // Soft Separation Settings
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation")
    bool bEnableSoftSeparation = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation")
    bool bDebugSoftSeparation = false;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation")
    float SoftSeparationMarginCm = 30.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation")
    float SoftSeparationStrength = 40000.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation")
    float SoftSeparationMaxForce = 90000.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation")
    float SoftSeparationDamping = 2.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation")
    float SoftSeparationResponse = 6.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Ship|SoftSeparation")
    int32 SoftSeparationMaxActors = 6;

    // NAVIGATION_TODO_REMOVE
    UFUNCTION(BlueprintCallable)
    void ApplySteeringForce(FVector TargetLocation, float DeltaTime);

    UFUNCTION(BlueprintCallable, Category="Ship|ManualControl")
    void SetManualRotationInput(float Pitch, float Yaw, float Roll);

    UFUNCTION(BlueprintCallable, Category="Ship|ManualControl")
    void StepThrottleUp();

    UFUNCTION(BlueprintCallable, Category="Ship|ManualControl")
    void StepThrottleDown();

private:
    bool EnsureShipController();
    void ApplyManualControl(float DeltaTime);

    bool bLoggedMissingController = false;

    bool bManualControl = false;
    int32 ManualThrottleStep = 0;
    float ManualPitchInput = 0.f;
    float ManualYawInput = 0.f;
    float ManualRollInput = 0.f;
    TWeakObjectPtr<AAIShipController> StoredAIController;

    float DebugMessageAccumulator = 0.f;

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

    // Soft Separation Force Application
    void ApplySoftSeparation(float DeltaTime);
};
