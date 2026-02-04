#include "Ship.h"
#include "AIShipController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "ShipNavComponent.h"
#include "ShipVitalityComponent.h"

AShip::AShip()
{
    PrimaryActorTick.bCanEverTick = true;

    // NAVIGATION_TODO_REMOVE
    AIControllerClass = AAIShipController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    ShipBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipBase"));
    SetRootComponent(ShipBase);

    ShipBase->SetSimulatePhysics(true);
    ShipBase->SetEnableGravity(false);
    ShipBase->SetCollisionProfileName(TEXT("Pawn"));

    ShipRadius = CreateDefaultSubobject<USphereComponent>(TEXT("ShipRadius"));
    ShipRadius->SetupAttachment(ShipBase);

    ShipNav = CreateDefaultSubobject<UShipNavComponent>(TEXT("ShipNav"));

    Vitality = CreateDefaultSubobject<UShipVitalityComponent>(TEXT("Vitality"));

    // NAVIGATION_TODO_REMOVE
    ShipRadius->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ShipRadius->SetCollisionProfileName(TEXT("Pawn"));
    ShipRadius->SetSphereRadius(300.f);

    ShipRadius->OnComponentBeginOverlap.AddDynamic(this, &AShip::HandleShipRadiusBeginOverlap);
    ShipRadius->OnComponentEndOverlap.AddDynamic(this, &AShip::HandleShipRadiusEndOverlap);
}

void AShip::BeginPlay()
{
    Super::BeginPlay();

    // NAVIGATION_TODO_REMOVE
    ShipController = Cast<AAIShipController>(GetController());
    if (!ShipController)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShipController not found on %s"), *GetName());
        bLoggedMissingController = true;
    }
}

FVector AShip::ComputeOrbitGoal(const FVector& Center, float DeltaTime)
{
    const AActor* CurTarget = TargetActor;
    if (!CurTarget || EffectiveRange <= 0.0f)
    {
        bOrbitInitialized = false;
        return Center;
    }

const bool bTargetChanged = (LastOrbitTarget.Get() != CurTarget);

FVector Axis = OrbitAxis.GetSafeNormal();
if (Axis.IsNearlyZero()) Axis = FVector::UpVector;

if (bAutoOrbitAxis)
{
    // Pick axis once per ship+target (or when re-entering orbit), then reuse
    if (bTargetChanged || !bOrbitInitialized)
    {
        const uint32 ShipSeed = (OrbitSeed != 0) ? uint32(OrbitSeed) : uint32(GetUniqueID());
        const uint32 Seed = HashCombine(ShipSeed, uint32(CurTarget->GetUniqueID()));
        FRandomStream Rng(Seed ^ 0xA31F2D9Bu);

        const float MinTiltRad = FMath::DegreesToRadians(OrbitAxisMinTiltDeg);
        const float MaxUpDot = FMath::Cos(MinTiltRad);

        FVector Cand = FVector::UpVector;
        for (int32 i = 0; i < 8; ++i)
        {
            Cand = Rng.VRand(); // unit vector
            const float UpDot = FMath::Abs(FVector::DotProduct(Cand, FVector::UpVector));
            if (UpDot <= MaxUpDot && !Cand.IsNearlyZero())
                break;
        }

        Axis = Cand.GetSafeNormal();
        if (Axis.IsNearlyZero()) Axis = FVector::UpVector;

        LastOrbitAxis = Axis;
        LastOrbitTarget = const_cast<AActor*>(CurTarget);
        bOrbitInitialized = false; // force basis + angle re-init on new plane
    }
    else
    {
        Axis = LastOrbitAxis.GetSafeNormal();
        if (Axis.IsNearlyZero()) Axis = FVector::UpVector;
    }
}
else
{
    // Manual axis mode: changing OrbitAxis resets orbit
    const bool bAxisChanged = !Axis.Equals(LastOrbitAxis, 0.01f);
    if (bTargetChanged || bAxisChanged)
    {
        bOrbitInitialized = false;
        LastOrbitTarget = const_cast<AActor*>(CurTarget);
        LastOrbitAxis = Axis;
    }
}

#if !UE_BUILD_SHIPPING
if (bDebugOrbit && GetWorld())
{
    DrawDebugLine(GetWorld(), Center, Center + Axis * 800.f, FColor::Green, false, 0.f, 0, 2.f);
}
#endif

    const FVector ShipPos = GetActorLocation();
    const FVector ToShip = ShipPos - Center;

    FVector PlaneVec = ToShip - Axis * FVector::DotProduct(ToShip, Axis);
    float PlaneDist = PlaneVec.Size();

    if (PlaneVec.IsNearlyZero())
    {
        FVector Fwd = GetActorForwardVector();
        PlaneVec = Fwd - Axis * FVector::DotProduct(Fwd, Axis);
        if (PlaneVec.IsNearlyZero()) PlaneVec = FVector::RightVector;
        PlaneDist = PlaneVec.Size();
    }

    const FVector RadialDir = PlaneVec.GetSafeNormal();
    const float DesiredR = EffectiveRange;
    const float EnterTol = FMath::Max(50.0f, DesiredR * OrbitEnterToleranceMultiplier);

    // Approach ring first
    if (PlaneDist > (DesiredR + EnterTol))
    {
        OrbitBasisX = RadialDir;
        OrbitBasisY = FVector::CrossProduct(Axis, OrbitBasisX).GetSafeNormal();
        if (OrbitBasisY.IsNearlyZero()) OrbitBasisY = FVector::RightVector;

        bOrbitInitialized = true;
        return Center + RadialDir * DesiredR;
    }

    if (!bOrbitInitialized)
    {
        OrbitBasisX = RadialDir;
        OrbitBasisY = FVector::CrossProduct(Axis, OrbitBasisX).GetSafeNormal();
        if (OrbitBasisY.IsNearlyZero()) OrbitBasisY = FVector::RightVector;

        // Deterministic random start angle per ship+target (stable across runs)
        const uint32 ShipSeed = (OrbitSeed != 0) ? uint32(OrbitSeed) : uint32(GetUniqueID());
        const uint32 Seed = HashCombine(ShipSeed, uint32(CurTarget->GetUniqueID()));
        FRandomStream Rng(Seed);
        OrbitAngleRad = Rng.FRandRange(-PI, PI);
        bOrbitInitialized = true;
    }

    const float Sign = bOrbitClockwise ? -1.0f : 1.0f;
    const float Omega = FMath::DegreesToRadians(OrbitAngularSpeedDeg) * Sign;
    OrbitAngleRad = FMath::UnwindRadians(OrbitAngleRad + Omega * FMath::Max(DeltaTime, 0.0f));

    float S=0.f, C=1.f;
    FMath::SinCos(&S, &C, OrbitAngleRad);

    const FVector Offset = (OrbitBasisX * C + OrbitBasisY * S) * DesiredR;
    return Center + Offset;
}

void AShip::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    EnsureShipController();
    if (!ShipController && !bLoggedMissingController)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShipController not found on %s"), *GetName());
        bLoggedMissingController = true;
    }

    // NAVIGATION_TODO_REMOVE
    const FVector GoalCenter = GetGoalLocation();
    FVector Goal = GoalCenter;
    bool bMovingGoal = false;

    if (TargetActor && bOrbitTarget && EffectiveRange > 0.0f)
    {
        Goal = ComputeOrbitGoal(GoalCenter, DeltaTime);
        bMovingGoal = true;
    }

    const FVector ActorLocation = GetActorLocation();
#if !UE_BUILD_SHIPPING
    if ((Goal - ActorLocation).IsNearlyZero(1.f))
    {
        DebugMessageAccumulator += DeltaTime;
        if (DebugMessageAccumulator >= 1.f)
        {
            DebugMessageAccumulator = 0.f;
            UE_LOG(LogTemp, Warning, TEXT("Ship %s has goal equal to self."), *GetName());
        }
    }
#endif
    const float ShipRadiusCm = ShipRadius ? ShipRadius->GetScaledSphereRadius() : 300.f;
    FVector SteeringTarget = Goal;
    if (ShipController && ShipController->IsInsideSafetyMargin())
    {
        SteeringTarget = ShipController->ComputeEscapeTarget(ActorLocation, ShipRadius);
#if !UE_BUILD_SHIPPING
        if (SteeringTarget.Equals(ActorLocation, 1.f))
        {
            UE_LOG(LogTemp, Warning, TEXT("InsideSafetyMargin but EscapeTarget == Self (ObstacleComp invalid?)"));
        }
#endif
    }
    else if (ShipNav && (!ShipController || !ShipController->IsUnstucking()))
    {
        ShipNav->TickNav(DeltaTime, Goal, ShipRadiusCm, bMovingGoal);
        SteeringTarget = ShipNav->GetNavTarget(Goal);
    }

    ApplySteeringForce(SteeringTarget, DeltaTime);

#if !UE_BUILD_SHIPPING
    if (bDebugOrbit && GetWorld())
    {
        DrawDebugPoint(GetWorld(), Goal, 14.f, FColor::Yellow, false, 0.f, 0);
        DrawDebugPoint(GetWorld(), SteeringTarget, 14.f, FColor::Cyan, false, 0.f, 0);
    }
#endif

    // Rotation
    // NAVIGATION_TODO_REMOVE
    if (ShipController)
    {
        ShipController->ApplyShipRotation(SteeringTarget);
    }

#if !UE_BUILD_SHIPPING
    if (ShipNav)
    {
        //DrawDebugPoint(GetWorld(), SteeringTarget, 16.0f, FColor::Cyan, false, 0.0f, 0);
    }
#endif

    // Apply soft separation forces after steering
    ApplySoftSeparation(DeltaTime);
}

void AShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AShip::NotifyHit(class UPrimitiveComponent* MyComp, class AActor* Other, class UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit)
{
    Super::NotifyHit(MyComp, Other, OtherComp, bSelfMoved, HitLocation, HitNormal, NormalImpulse, Hit);
    
    // Apply tiny corrective push force only on actual collision hits
    if (bEnableSoftSeparation && ShipBase && OtherComp)
    {
        // Only apply push force on direct hits with blocking components
        const bool bBlocksStatic = OtherComp->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block;
        const bool bBlocksDynamic = OtherComp->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block;
        const bool bBlocksPhysics = OtherComp->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Block;
        const bool bIsPawn = Other && Other->IsA<APawn>();
        
        if (bBlocksStatic || bBlocksDynamic || bBlocksPhysics || bIsPawn)
        {
            const float Mass = ShipBase->GetMass();
            
            // Calculate impact speed from normal impulse
            const float ImpactSpeed = NormalImpulse.Size() / FMath::Max(Mass, 1.0f);
            
            // Do NOT apply hit push if impact speed is small
            if (ImpactSpeed > 10.0f) // Only apply for significant impacts
            {
                // Tiny corrective push force - VERY LOW (10-20% of SoftSeparationMaxForce)
                const float MaxHitForce = SoftSeparationMaxForce * 0.2f; // 20% of max separation force
                const float HitForceStrength = FMath::Min(ImpactSpeed * 50.0f, MaxHitForce); // Scale by impact speed, hard clamp
                
                // Direction = HitNormal (outward from collision surface)
                FVector HitForce = HitNormal * HitForceStrength;
                
                // Apply damping to avoid bounce - prefer AddForce over AddImpulse
                // Apply immediate force on hit (no smoothing for hit response)
                ShipBase->AddForce(HitForce, NAME_None, true);
                
#if !UE_BUILD_SHIPPING
                if (bDebugSoftSeparation && GetWorld())
                {
                    DrawDebugDirectionalArrow(GetWorld(), HitLocation, HitLocation + HitNormal * 100.0f, 5.0f, FColor::Magenta, false, 0.1f, 0, 2.0f);
                }
#endif
            }
        }
    }
}

void AShip::HandleShipRadiusBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherComp)
    {
        return;
    }

    // Update overlap filtering logic to also accept Pawns as blockers
    const bool bBlocksStatic = OtherComp->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block;
    const bool bBlocksDynamic = OtherComp->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block;
    const bool bBlocksPhysics = OtherComp->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Block;
    const bool bIsPawn = OtherActor && OtherActor->IsA<APawn>();
    
    if (!bBlocksStatic && !bBlocksDynamic && !bBlocksPhysics && !bIsPawn)
    {
        return;
    }

    // Add to soft separation bookkeeping (avoid duplicates)
    if (OtherComp != ShipRadius && OtherComp != ShipBase)
    {
        // Check if component already exists (avoid duplicates)
        bool bAlreadyExists = false;
        for (const auto& WeakComp : SoftOverlapComps)
        {
            if (WeakComp.IsValid() && WeakComp.Get() == OtherComp)
            {
                bAlreadyExists = true;
                break;
            }
        }
        
        if (!bAlreadyExists)
        {
            SoftOverlapComps.Add(TWeakObjectPtr<UPrimitiveComponent>(OtherComp));
        }
    }

    if (EnsureShipController())
    {
        ShipController->SetCurrentObstacleComp(OtherComp);
    }
}

void AShip::HandleShipRadiusEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!OtherComp || !EnsureShipController())
    {
        return;
    }

    // Remove from soft separation bookkeeping
    for (int32 i = SoftOverlapComps.Num() - 1; i >= 0; --i)
    {
        if (SoftOverlapComps[i].IsValid() && SoftOverlapComps[i].Get() == OtherComp)
        {
            SoftOverlapComps.RemoveAtSwap(i);
            break; // Found and removed, no need to continue
        }
    }

    if (ShipController->GetCurrentObstacleComp() == OtherComp)
    {
        ShipController->SetCurrentObstacleComp(nullptr);
    }
}

UStaticMeshComponent* AShip::GetShipBase() const
{
    return ShipBase;
}

// NAVIGATION_TODO_REMOVE
FVector AShip::GetGoalLocation() const
{
    if (ShipController)
    {
        return ShipController->GetFocusLocation();
    }

    return GetActorLocation();
}

// NAVIGATION_TODO_REMOVE
void AShip::ApplySteeringForce(FVector TargetLocation, float DeltaTime)
{
    if (!ShipBase || DeltaTime <= 0.f)
    {
        return;
    }

    const FVector ActorLocation = GetActorLocation();
    const FVector ToTarget = (TargetLocation - ActorLocation).GetSafeNormal();
    if (ToTarget.IsNearlyZero())
    {
#if !UE_BUILD_SHIPPING
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                1.f,
                FColor::Yellow,
                TEXT("No valid target (goal == self)")
            );
        }
#endif
        return;
    }

    const FVector Forward = ShipBase->GetForwardVector();
    const float Heading = FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.f, 1.f);

    const float ThrottleTarget = FMath::Clamp(Heading, MinThrottle, 1.f);
    CurrentThrottle = FMath::FInterpTo(CurrentThrottle, ThrottleTarget, DeltaTime, ThrottleInterpSpeed);

    const float Mass = ShipBase->GetMass();
    const FVector Force = Forward * MaxForwardForce * CurrentThrottle * Mass;
    ShipBase->AddForce(Force, NAME_None, false);

    const FVector Velocity = ShipBase->GetPhysicsLinearVelocity();
    const FVector LateralVelocity = Velocity - Forward * FVector::DotProduct(Velocity, Forward);
    ShipBase->AddForce(-LateralVelocity * LateralDamping * Mass, NAME_None, false);

/* #if !UE_BUILD_SHIPPING
    //DrawDebugLine(GetWorld(), ActorLocation, TargetLocation, FColor::Green, false, 0.f, 0, 2.f);
    DebugMessageAccumulator += DeltaTime;
    if (DebugMessageAccumulator >= 1.f)
    {
        DebugMessageAccumulator = 0.f;
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                1.f,
                FColor::Green,
                FString::Printf(
                    TEXT("SteeringTarget: %s | Throttle: %.2f"),
                    *TargetLocation.ToString(),
                    CurrentThrottle
                )
            );
        }
    }
#endif */
}

bool AShip::EnsureShipController()
{
    if (!ShipController)
    {
        ShipController = Cast<AAIShipController>(GetController());
    }
    return ShipController != nullptr;
}

void AShip::ApplySoftSeparation(float DeltaTime)
{
    // Early-out conditions - avoid computation when disabled or invalid
    if (!bEnableSoftSeparation || !ShipBase || !ShipRadius || DeltaTime <= 0.0f)
    {
        return;
    }

    // Periodic cleanup of invalid entries to prevent memory leaks
    SoftSeparationCleanupCounter++;
    if (SoftSeparationCleanupCounter >= 60)
    {
        SoftOverlapComps.RemoveAllSwap([](const TWeakObjectPtr<UPrimitiveComponent>& W) { return !W.IsValid(); }, EAllowShrinking::No);
        SoftSeparationCleanupCounter = 0;
    }

    const float ShipRadiusScaled = ShipRadius->GetScaledSphereRadius();
    const float ActivationRadius = ShipRadiusScaled + SoftSeparationMarginCm;
    const FVector ShipPos = GetActorLocation();
    const FVector Velocity = ShipBase->GetPhysicsLinearVelocity();
    const float Mass = ShipBase->GetMass();

    // Reuse array to avoid allocations per tick - reserve space for efficiency
    OverlapDistancesCache.Reset();
    OverlapDistancesCache.Reserve(SoftOverlapComps.Num());
    
    // Process overlaps and find closest points
    for (const auto& WeakComp : SoftOverlapComps)
    {
        if (!WeakComp.IsValid())
        {
            continue;
        }

        UPrimitiveComponent* Comp = WeakComp.Get();
        if (!Comp || Comp == ShipBase || Comp == ShipRadius)
        {
            continue;
        }

        // Find closest point on collision to ShipBase location
        FVector ClosestPoint;
        if (Comp->GetClosestPointOnCollision(ShipPos, ClosestPoint))
        {
            const float Dist = FVector::Distance(ShipPos, ClosestPoint);
            // Only process if within activation radius
            if (Dist < ActivationRadius)
            {
                OverlapDistancesCache.Emplace(WeakComp, ClosestPoint, Dist);
            }
        }
    }

    // Sort by distance (closest first) and limit to max actors
    OverlapDistancesCache.Sort();
    const int32 NumToProcess = FMath::Min(OverlapDistancesCache.Num(), SoftSeparationMaxActors);

    FVector TotalForce = FVector::ZeroVector;

    // Process each overlap with branch-light force calculation
    for (int32 i = 0; i < NumToProcess; ++i)
    {
        const FOverlapDistanceInfo& Info = OverlapDistancesCache[i];
        const FVector ClosestPoint = Info.ClosestPoint;
        const float Dist = Info.Distance;

        // Compute penetration alpha with branchless operations where possible
        const float PenetrationRatio = (ActivationRadius - Dist) / ActivationRadius;
        const float PenetrationAlpha = FMath::Clamp(PenetrationRatio, 0.0f, 1.0f);
        
        // Apply smoothstep to PenetrationAlpha for smoother force response
        const float SmoothedAlpha = PenetrationAlpha * PenetrationAlpha * (3.0f - 2.0f * PenetrationAlpha);

        // Compute contact normal - skip if nearly zero
        const FVector ToClosest = ShipPos - ClosestPoint;
        const float DistSq = ToClosest.SizeSquared();
        if (DistSq < KINDA_SMALL_NUMBER)
        {
            continue;
        }
        
        const FVector N = ToClosest * FMath::InvSqrt(DistSq); // Faster normalization

        // Compute velocity going INTO the obstacle (branch-light calculation)
        const float IntoSpeed = (-N.X * Velocity.X) + (-N.Y * Velocity.Y) + (-N.Z * Velocity.Z); // Manual dot product for clarity

        // Apply BRAKING ONLY - remove velocity component going INTO obstacle
        // NO outward forces to prevent slingshot effects
        // Branch prediction friendly: likely to be > 0 for overlapping ships
        if (IntoSpeed > 0.0f)
        {
            // Force direction opposes the velocity component going into obstacle
            // Scale by: penetration depth, damping, and mass
            const float ForceMagnitude = IntoSpeed * SmoothedAlpha * SoftSeparationDamping * Mass;
            
            // Clamp force magnitude to prevent any large forces that could cause instability
            const float ClampedForceMagnitude = FMath::Min(ForceMagnitude, SoftSeparationMaxForce);
            
            // Apply force in the normal direction (braking direction)
            TotalForce.X += N.X * ClampedForceMagnitude;
            TotalForce.Y += N.Y * ClampedForceMagnitude;
            TotalForce.Z += N.Z * ClampedForceMagnitude;
        }
    }

    // Smooth the force over time for stable behavior
    // VInterpTo provides exponential smoothing that helps prevent jitter
    SmoothedSoftSeparationForce = FMath::VInterpTo(SmoothedSoftSeparationForce, TotalForce, DeltaTime, SoftSeparationResponse);

    // Apply force to physics body
    // NOTE: AddForce is NOT scaled by DeltaTime because:
    // 1. Physics force integration already handles time scaling internally
    // 2. DeltaTime scaling would make force response dependent on frame rate
    // 3. This ensures consistent force application regardless of frame rate
    ShipBase->AddForce(SmoothedSoftSeparationForce);

#if !UE_BUILD_SHIPPING
    if (bDebugSoftSeparation && GetWorld())
    {
        // Draw debug sphere showing the soft separation boundary
        DrawDebugSphere(GetWorld(), ShipPos, ActivationRadius, 12, FColor::Yellow, false, 0.0f, 0, 1.0f);
        
        // Draw debug line for the smoothed force vector (scaled for visibility)
        if (!SmoothedSoftSeparationForce.IsNearlyZero())
        {
            const FVector ForceEnd = ShipPos + SmoothedSoftSeparationForce.GetSafeNormal() * FMath::Min(SmoothedSoftSeparationForce.Size() * 0.01f, 100.0f);
            DrawDebugDirectionalArrow(GetWorld(), ShipPos, ForceEnd, 5.0f, FColor::Blue, false, 0.0f, 0, 2.0f);
        }
    }
    
    // Find max penetration alpha for logging
    float MaxPenetrationAlpha = 0.0f;
    for (int32 i = 0; i < NumToProcess; ++i)
    {
        const float Dist = OverlapDistancesCache[i].Distance;
        const float PenetrationAlpha = FMath::Clamp((ActivationRadius - Dist) / ActivationRadius, 0.0f, 1.0f);
        MaxPenetrationAlpha = FMath::Max(MaxPenetrationAlpha, PenetrationAlpha);
    }
    
    // Log debug information
    if (bDebugSoftSeparation)
    {
        UE_LOG(LogTemp, Log, TEXT("Ship SoftSeparation - MaxPenetrationAlpha: %.3f, AppliedForce: %.1f"), 
               MaxPenetrationAlpha, SmoothedSoftSeparationForce.Size());
               
#if !UE_BUILD_SHIPPING
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                0.1f,
                FColor::Cyan,
                FString::Printf(TEXT("MaxPenetrationAlpha: %.3f, AppliedForce: %.1f"), 
                               MaxPenetrationAlpha, SmoothedSoftSeparationForce.Size())
            );
        }
#endif
    }
#endif
}
