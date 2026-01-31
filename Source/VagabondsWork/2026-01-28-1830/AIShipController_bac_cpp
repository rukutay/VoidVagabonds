#include "AIShipController.h"
#include "Engine/OverlapResult.h"
#include "Components/PrimitiveComponent.h"
#include "Ship.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

void AAIShipController::DrawDebugTargetPoint(
    const FVector& Point,
    float Radius,
    FColor Color
) const
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    DrawDebugSphere(
        World,
        Point,
        Radius,
        12,
        Color,
        false,     // NOT persistent
        0.f,       // ONE FRAME
        0,
        1.5f
    );
}



FVector AAIShipController::GetFocusLocation()
{
    if (TargetActor) {
        // If a target actor is set, use its location as the destination focus
        FocusLocation = TargetActor->GetActorLocation();
    } else {
        // Fallback to player pawn if no specific target actor is assigned
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (PlayerPawn) {
            FocusLocation = PlayerPawn->GetActorLocation();
        }
    }
    return FocusLocation;
}

void AAIShipController::ApplyShipRotation(FVector TargetLocation)
{
    AShip* Ship = Cast<AShip>(GetPawn());
    if (!Ship) return;

    UStaticMeshComponent* ShipBase = Ship->GetShipBase();
    if (!ShipBase) return;

    const float DeltaTime = GetWorld()->GetDeltaSeconds();
    if (DeltaTime <= 0.f) return;

    const FVector ActorLocation = Ship->GetActorLocation();
    FVector ToTargetWorld = (TargetLocation - ActorLocation);
    if (ToTargetWorld.IsNearlyZero()) return;
    ToTargetWorld.Normalize();

    const FVector ToTargetLocal = ShipBase->GetComponentTransform()
        .InverseTransformVectorNoScale(ToTargetWorld);
    if (ToTargetLocal.IsNearlyZero()) return;

    const FVector ToTargetLocalDir = ToTargetLocal.GetSafeNormal();

    const float PitchErr = -FMath::RadiansToDegrees(
        FMath::Atan2(ToTargetLocalDir.Z, ToTargetLocalDir.X)
    );
    const float YawErr = FMath::RadiansToDegrees(
        FMath::Atan2(ToTargetLocalDir.Y, ToTargetLocalDir.X)
    );

    const FRotator CurrentRot = ShipBase->GetComponentRotation();

    // Proportional controller -> desired angular velocity (deg/sec)
    const float PitchKp = 0.5f;
    const float YawKp = 0.55f;

    const float MaxPitch = Ship->MaxPitchSpeed;
    const float MaxYaw = Ship->MaxYawSpeed;
    const float MaxRoll = Ship->MaxRollSpeed;
    const float MaxRollAngle = Ship->MaxRollAngle;

    const float DesiredPitchRate = FMath::Clamp(PitchErr * PitchKp, -MaxPitch, MaxPitch);
    const float DesiredYawRate = FMath::Clamp(YawErr * YawKp, -MaxYaw, MaxYaw);

    const float DesiredRollRate = 0.f;

    // Current angular velocity (deg/sec) in world space. Convert to local for axis control.
    const FVector CurAngVelWorld = ShipBase->GetPhysicsAngularVelocityInDegrees();
    const FVector CurAngVelLocal = ShipBase->GetComponentTransform()
        .InverseTransformVectorNoScale(CurAngVelWorld);

    const FVector TargetAngVelLocal(DesiredRollRate, DesiredPitchRate, DesiredYawRate);

    // Smooth angular velocity change
    const float PitchResponse = FMath::Max(Ship->PitchAccelSpeed, 0.1f);
    const float YawResponse = FMath::Max(Ship->YawAccelSpeed, 0.1f);
    const float RollResponse = FMath::Max(Ship->RollAccelSpeed, 0.1f);

    FVector NewAngVelLocal = CurAngVelLocal;
    NewAngVelLocal.X = FMath::FInterpTo(CurAngVelLocal.X, TargetAngVelLocal.X, DeltaTime, RollResponse);
    NewAngVelLocal.Y = FMath::FInterpTo(CurAngVelLocal.Y, TargetAngVelLocal.Y, DeltaTime, PitchResponse);
    NewAngVelLocal.Z = FMath::FInterpTo(CurAngVelLocal.Z, TargetAngVelLocal.Z, DeltaTime, YawResponse);

    const FVector NewAngVelWorld = ShipBase->GetComponentTransform()
        .TransformVectorNoScale(NewAngVelLocal);

    ShipBase->SetPhysicsAngularVelocityInDegrees(NewAngVelWorld, false);

    // IMPORTANT: remove debug draw in shipping/perf
    // DrawDebugTargetPoint(TargetLocation, 500, FColor::Green);
}


bool AAIShipController::IsPointFree(
    UWorld* World,
    const FVector& Point,
    float Radius,
    const AActor* IgnoreActor,
    bool bStaticOnly,
    bool bDynamicOnly
)
{
    if (!World) return false;

    if (bStaticOnly && bDynamicOnly)
    {
        bStaticOnly = false;
        bDynamicOnly = false;
    }

    FCollisionObjectQueryParams Obj;
    if (!bDynamicOnly)
    {
        Obj.AddObjectTypesToQuery(ECC_WorldStatic);
        Obj.AddObjectTypesToQuery(ECC_WorldDynamic);
    }
    if (!bStaticOnly)
    {
        Obj.AddObjectTypesToQuery(ECC_PhysicsBody);
        Obj.AddObjectTypesToQuery(ECC_Pawn);
    }

    FCollisionQueryParams Params(SCENE_QUERY_STAT(AvoidPointFree), false);
    Params.bTraceComplex = false;
    if (IgnoreActor) Params.AddIgnoredActor(IgnoreActor);

    const bool bOverlap = World->OverlapAnyTestByObjectType(
        Point,
        FQuat::Identity,
        Obj,
        FCollisionShape::MakeSphere(Radius),
        Params
    );

    return !bOverlap;
}

bool AAIShipController::HasLineOfSight(
    UWorld* World,
    const FVector& From,
    const FVector& To,
    const AActor* IgnoreActor
)
{
    if (!World) return false;

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AvoidLOS), false);
    Params.bTraceComplex = false;
    if (IgnoreActor) Params.AddIgnoredActor(IgnoreActor);

    // Visibility channel is fine for "is it blocked at all"
    const bool bHit = World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
    return !bHit;
}

FVector AAIShipController::CellToWorld(
    const FVector& Center,
    int32 ix, int32 iy, int32 iz,
    int32 Half,
    float Gap
)
{
    return Center + FVector(
        float(ix - Half) * Gap,
        float(iy - Half) * Gap,
        float(iz - Half) * Gap
    );
}

FVector AAIShipController::GetBestGridPointTowardGoal(
    AAIShipController* Controller,
    USphereComponent* ShipRadius,
    FVector GridCenter,
    FVector GoalLocation,
    int32 NumCells,
    float GapMultiplier,
    bool bDrawDebug,
    bool bStaticOnly,
    bool bDynamicOnly
)
{
    if (!Controller || !ShipRadius) return FVector::ZeroVector;

    APawn* Pawn = Controller->GetPawn();
    if (!Pawn) return FVector::ZeroVector;

    UWorld* World = Pawn->GetWorld();
    if (!World) return FVector::ZeroVector;

    const float Radius = ShipRadius->GetScaledSphereRadius();
    const float Gap = FMath::Max(50.f, Radius * GapMultiplier);

    NumCells = FMath::Clamp(NumCells, 3, 11);
    if ((NumCells % 2) == 0) NumCells += 1;
    const int32 Half = NumCells / 2;

    const FVector From = Pawn->GetActorLocation();
    FVector ToGoalDir = (GoalLocation - From);
    if (!ToGoalDir.Normalize())
    {
        // already at goal, just stay
        return From;
    }

    FVector Best = FVector::ZeroVector;
    FVector SecondBest = FVector::ZeroVector;
    float BestScore = TNumericLimits<float>::Lowest();
    float SecondScore = TNumericLimits<float>::Lowest();

    for (int32 ix = 0; ix < NumCells; ++ix)
    {
        for (int32 iy = 0; iy < NumCells; ++iy)
        {
            for (int32 iz = 0; iz < NumCells; ++iz)
            {
                const FVector P = CellToWorld(GridCenter, ix, iy, iz, Half, Gap);

                if (!IsPointFree(World, P, Radius, Pawn, bStaticOnly, bDynamicOnly))
                    continue;

                const float DistToGoal2 = FVector::DistSquared(P, GoalLocation);

                FVector Dir = (P - From);
                const float DirLen2 = Dir.SizeSquared();
                const float Align = (DirLen2 > KINDA_SMALL_NUMBER) ? FVector::DotProduct(Dir / FMath::Sqrt(DirLen2), ToGoalDir) : 0.f;

                const float Score = (-DistToGoal2 * 0.000001f) + (Align * 2.0f);

                if (Score > BestScore)
                {
                    SecondScore = BestScore;
                    SecondBest = Best;

                    BestScore = Score;
                    Best = P;
                }
                else if (Score > SecondScore)
                {
                    SecondScore = Score;
                    SecondBest = P;
                }

#if !(UE_BUILD_SHIPPING)
                if (bDrawDebug)
                {
                    DrawDebugSphere(World, P, 40.f, 8, FColor::Cyan, false, 0.25f, 0, 0.5f);
                }
#endif
            }
        }
    }

    // LOS only 1–2 times
    if (!Best.IsNearlyZero() && HasLineOfSight(World, From, Best, Pawn))
    {
        return Best;
    }
    if (!SecondBest.IsNearlyZero() && HasLineOfSight(World, From, SecondBest, Pawn))
    {
        return SecondBest;
    }

    // Fallback: move away from the obstacle center, but still generally toward goal
    // (cheap, no extra traces)
    // Fallback: create a tangential "go around" direction instead of backing off
    FVector Away = (From - GridCenter);
    if (!Away.Normalize())
    {
        Away = -ToGoalDir;
    }

    // Tangent direction around obstacle face.
    // Prefer tangent that still has positive dot toward goal.
    FVector Tangent = FVector::CrossProduct(FVector::UpVector, Away);
    if (!Tangent.Normalize())
    {
        Tangent = FVector::CrossProduct(FVector::RightVector, Away);
        Tangent.Normalize();
    }

    // Pick side that goes more toward goal
    if (FVector::DotProduct(Tangent, ToGoalDir) < 0.f)
    {
        Tangent *= -1.f;
    }

    // Step sideways + slightly away + slightly forward (this makes ships slide around big obstacles)
    const FVector Fallback =
        GridCenter
        + Tangent * (Radius * 6.f)
        + Away    * (Radius * 1.5f)
        + ToGoalDir * (Radius * 2.f);

    return Fallback;

}


TArray<FVector> AAIShipController::FindPathAStar3D(
    AAIShipController* Controller,
    USphereComponent* ShipRadius,
    const FVector& StartWorld,
    const FVector& GoalWorld,
    const FVector& GridCenter,
    int32 NumCells,
    float GapMultiplier,
    int32 MaxExpandedNodes,
    bool bDrawDebug,
    bool bStaticOnly,
    bool bDynamicOnly
)
{
    TArray<FVector> OutPath;

    if (!Controller || !ShipRadius) return OutPath;

    APawn* Pawn = Controller->GetPawn();
    if (!Pawn) return OutPath;

    UWorld* World = Pawn->GetWorld();
    if (!World) return OutPath;

    const float Radius = ShipRadius->GetScaledSphereRadius();
    const float Gap = FMath::Max(80.f, Radius * GapMultiplier);

    NumCells = FMath::Clamp(NumCells, 5, 17);
    if ((NumCells % 2) == 0) NumCells += 1;
    const int32 Half = NumCells / 2;

    auto WorldToCell = [&](const FVector& P, int32& ox, int32& oy, int32& oz) -> bool
    {
        const FVector Local = (P - GridCenter) / Gap;
        ox = FMath::RoundToInt(Local.X) + Half;
        oy = FMath::RoundToInt(Local.Y) + Half;
        oz = FMath::RoundToInt(Local.Z) + Half;
        return (ox >= 0 && ox < NumCells && oy >= 0 && oy < NumCells && oz >= 0 && oz < NumCells);
    };

    auto Key = [&](int32 x, int32 y, int32 z) -> int64
    {
        // Pack into 64-bit key (NumCells <= 17 so safe)
        return (int64(x) << 40) | (int64(y) << 20) | int64(z);
    };

    struct FNode
    {
        float G = 0.f;
        float F = 0.f;
        int64 Parent = -1;
        bool bClosed = false;
        int32 X = 0, Y = 0, Z = 0;
    };

    TMap<int64, FNode> Nodes;
    TArray<int64> Open;

    int32 sx, sy, sz, gx, gy, gz;
    if (!WorldToCell(StartWorld, sx, sy, sz)) return OutPath;
    if (!WorldToCell(GoalWorld,  gx, gy, gz)) return OutPath;

    const int64 StartKey = Key(sx, sy, sz);
    const int64 GoalKey  = Key(gx, gy, gz);

    auto Heuristic = [&](int32 x, int32 y, int32 z) -> float
    {
        // Euclidean in cell space
        const float dx = float(x - gx);
        const float dy = float(y - gy);
        const float dz = float(z - gz);
        return FMath::Sqrt(dx*dx + dy*dy + dz*dz);
    };

    auto PushOpen = [&](int64 K)
    {
        Open.Add(K);
    };

    auto PopBestOpen = [&]() -> int64
    {
        int32 BestIdx = 0;
        float BestF = TNumericLimits<float>::Max();
        for (int32 i = 0; i < Open.Num(); ++i)
        {
            const int64 K = Open[i];
            const FNode* N = Nodes.Find(K);
            if (!N) continue;
            if (N->F < BestF)
            {
                BestF = N->F;
                BestIdx = i;
            }
        }
        const int64 OutK = Open[BestIdx];
        Open.RemoveAtSwap(BestIdx);
        return OutK;
    };

    // Seed start node (even if it's overlapping, still allow)
    {
        FNode& S = Nodes.FindOrAdd(StartKey);
        S.X = sx; S.Y = sy; S.Z = sz;
        S.G = 0.f;
        S.F = Heuristic(sx, sy, sz);
        S.Parent = -1;
        S.bClosed = false;
        PushOpen(StartKey);
    }

    const int32 NeighborOffsets[26][3] = {
        {-1,-1,-1},{-1,-1, 0},{-1,-1, 1},
        {-1, 0,-1},{-1, 0, 0},{-1, 0, 1},
        {-1, 1,-1},{-1, 1, 0},{-1, 1, 1},

        { 0,-1,-1},{ 0,-1, 0},{ 0,-1, 1},
        { 0, 0,-1},           { 0, 0, 1},
        { 0, 1,-1},{ 0, 1, 0},{ 0, 1, 1},

        { 1,-1,-1},{ 1,-1, 0},{ 1,-1, 1},
        { 1, 0,-1},{ 1, 0, 0},{ 1, 0, 1},
        { 1, 1,-1},{ 1, 1, 0},{ 1, 1, 1}
    };

    int32 Expanded = 0;

    while (Open.Num() > 0 && Expanded < MaxExpandedNodes)
    {
        const int64 CurrentKey = PopBestOpen();
        FNode* Current = Nodes.Find(CurrentKey);
        if (!Current || Current->bClosed) continue;

        Current->bClosed = true;
        Expanded++;

        if (CurrentKey == GoalKey)
            break;

        const FVector CurrentPos = CellToWorld(GridCenter, Current->X, Current->Y, Current->Z, Half, Gap);

        for (int32 ni = 0; ni < 26; ++ni)
        {
            const int32 nx = Current->X + NeighborOffsets[ni][0];
            const int32 ny = Current->Y + NeighborOffsets[ni][1];
            const int32 nz = Current->Z + NeighborOffsets[ni][2];

            if (nx < 0 || nx >= NumCells || ny < 0 || ny >= NumCells || nz < 0 || nz >= NumCells)
                continue;

            const int64 NK = Key(nx, ny, nz);

            FNode* N = Nodes.Find(NK);
            if (N && N->bClosed)
                continue;

            const FVector NPos = CellToWorld(GridCenter, nx, ny, nz, Half, Gap);

            // Occupancy
            if (!IsPointFree(World, NPos, Radius, Pawn, bStaticOnly, bDynamicOnly))
                continue;

            // Edge validity: LOS from current cell to neighbor reduces "squeezing through corners"
            if (!HasLineOfSight(World, CurrentPos, NPos, Pawn))
                continue;

            const float StepCost = FVector::Dist(CurrentPos, NPos) / Gap; // ~1..sqrt(3)
            const float TentG = Current->G + StepCost;

            if (!N)
            {
                FNode NewNode;
                NewNode.X = nx; NewNode.Y = ny; NewNode.Z = nz;
                NewNode.G = TentG;
                NewNode.F = TentG + Heuristic(nx, ny, nz);
                NewNode.Parent = CurrentKey;
                NewNode.bClosed = false;
                Nodes.Add(NK, NewNode);
                PushOpen(NK);
            }
            else if (TentG < N->G)
            {
                N->G = TentG;
                N->F = TentG + Heuristic(nx, ny, nz);
                N->Parent = CurrentKey;
                PushOpen(NK); // re-add; PopBestOpen ignores closed
            }
        }
    }

    // Reconstruct if goal reached
    const FNode* GoalNode = Nodes.Find(GoalKey);
    if (!GoalNode || !GoalNode->bClosed)
        return OutPath;

    TArray<FVector> Reverse;
    int64 Walk = GoalKey;

    while (Walk != -1)
    {
        const FNode* N = Nodes.Find(Walk);
        if (!N) break;
        Reverse.Add(CellToWorld(GridCenter, N->X, N->Y, N->Z, Half, Gap));
        Walk = N->Parent;
    }

    for (int32 i = Reverse.Num() - 1; i >= 0; --i)
        OutPath.Add(Reverse[i]);

    // Optional: prune very close points
    {
        TArray<FVector> Pruned;
        const float MinStep2 = FMath::Square(Radius * 0.75f);

        FVector Last = StartWorld;
        for (const FVector& P : OutPath)
        {
            if (FVector::DistSquared(Last, P) >= MinStep2)
            {
                Pruned.Add(P);
                Last = P;
            }
        }
        OutPath = Pruned;
    }

#if !(UE_BUILD_SHIPPING)
    if (bDrawDebug)
    {
        for (int32 i = 0; i < OutPath.Num(); ++i)
        {
            DrawDebugSphere(World, OutPath[i], 55.f, 10, FColor::Yellow, false, 0.5f, 0, 1.0f);
            if (i + 1 < OutPath.Num())
            {
                DrawDebugLine(World, OutPath[i], OutPath[i+1], FColor::Yellow, false, 0.5f, 0, 2.0f);
            }
        }
    }
#endif

    return OutPath;
}
