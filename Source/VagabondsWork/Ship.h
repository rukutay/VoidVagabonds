#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Ship.generated.h"

class AAIShipController;
class UShipNavComponent;

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

    UFUNCTION(BlueprintCallable, Category="AI|Navigation")
    AActor* GetTargetActor() const { return TargetActor; }

    // ---------------- Controller ----------------
    // NAVIGATION_TODO_REMOVE
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI", meta=(AllowPrivateAccess="true"))
    AAIShipController* ShipController = nullptr;

    // ---------------- Components ----------------
    UFUNCTION(BlueprintCallable)
    UStaticMeshComponent* GetShipBase() const;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    UStaticMeshComponent* ShipBase = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    USphereComponent* ShipRadius = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI")
    UShipNavComponent* ShipNav = nullptr;

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

private:
    bool EnsureShipController();

    bool bLoggedMissingController = false;

    float DebugMessageAccumulator = 0.f;

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
