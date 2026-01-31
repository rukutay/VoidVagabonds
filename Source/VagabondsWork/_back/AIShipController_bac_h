#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Components/SphereComponent.h"
#include "AIShipController.generated.h"

class AShip;

UCLASS()
class VAGABONDSWORK_API AAIShipController : public AAIController
{
    GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AI")
    AActor* TargetActor = nullptr;

    // ---------------- Focus / Goal ----------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Movement")
    FVector FocusLocation = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable)
    FVector GetFocusLocation();

    UFUNCTION(BlueprintCallable, Category="Rotation")
    void ApplyShipRotation(FVector TargetLocation);

    void DrawDebugTargetPoint(
        const FVector& Point,
        float Radius = 50.f,
        FColor Color = FColor::Green
    ) const;

    // ---------------- Avoidance (dynamic obstacles: fast local grid pick) ----------------
    UFUNCTION(BlueprintCallable, Category="AI|Navigation")
    FVector GetBestGridPointTowardGoal(
        AAIShipController* Controller,
        USphereComponent* ShipRadius,
        FVector GridCenter,
        FVector GoalLocation,
        int32 NumCells = 5,
        float GapMultiplier = 2.0f,
        bool bDrawDebug = false,
        bool bStaticOnly = false,
        bool bDynamicOnly = false
    );

    // ---------------- Avoidance (static/physics: 3D A* in coarse grid) ----------------
    UFUNCTION(BlueprintCallable, Category="AI|Navigation")
    TArray<FVector> FindPathAStar3D(
        AAIShipController* Controller,
        USphereComponent* ShipRadius,
        const FVector& StartWorld,
        const FVector& GoalWorld,
        const FVector& GridCenter,
        int32 NumCells = 9,
        float GapMultiplier = 2.0f,
        int32 MaxExpandedNodes = 1500,
        bool bDrawDebug = false,
        bool bStaticOnly = false,
        bool bDynamicOnly = false
    );

private:
    // ---- Helpers ----
    static bool IsPointFree(
        UWorld* World,
        const FVector& Point,
        float Radius,
        const AActor* IgnoreActor,
        bool bStaticOnly = false,
        bool bDynamicOnly = false
    );

    static bool HasLineOfSight(
        UWorld* World,
        const FVector& From,
        const FVector& To,
        const AActor* IgnoreActor
    );

    static FVector CellToWorld(
        const FVector& Center,
        int32 ix, int32 iy, int32 iz,
        int32 Half,
        float Gap
    );
};
