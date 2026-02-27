#include "AIShipController.h"
#include "Ship.h"
#include "NavStaticBig.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "Components/PrimitiveComponent.h"

void AAIShipController::BeginPlay()
{
    Super::BeginPlay();

    StopPatrol(false);

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            StuckCheckTimerHandle,
            this,
            &AAIShipController::HandleStuckCheck,
            StuckCheckInterval,
            true);

        GetWorld()->GetTimerManager().SetTimer(
            SafetyMarginTimerHandle,
            this,
            &AAIShipController::HandleSafetyMarginCheck,
            SafetyMarginCheckInterval,
            true);
    }
}

void AAIShipController::PruneInvalidPatrolHead()
{
    while (PatrolRoute.Num() > 0 && !IsValid(PatrolRoute[0]))
    {
        PatrolRoute.RemoveAt(0, 1, EAllowShrinking::No);
    }

    CurrentPatrolTarget = PatrolRoute.Num() > 0 ? PatrolRoute[0] : nullptr;

    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->TargetActor = CurrentPatrolTarget.Get();
    }

    if (PatrolRoute.Num() == 0)
    {
        StopPatrol(false);
    }
}

void AAIShipController::StartPatrol(const TArray<ANavStaticBig*>& NavStaticActors, float InPointDelaySeconds)
{
    StopPatrol(true);

    PatrolPointDelaySeconds = FMath::Max(0.0f, InPointDelaySeconds);
    PatrolRoute.Reset();
    PatrolRoute.Reserve(NavStaticActors.Num());
    for (ANavStaticBig* RouteActor : NavStaticActors)
    {
        if (!IsValid(RouteActor))
        {
            continue;
        }
        PatrolRoute.Add(RouteActor);
    }

    PruneInvalidPatrolHead();
    if (PatrolRoute.Num() == 0)
    {
        ActionMode = EActionMode::Idle;
        UE_LOG(LogTemp, Warning, TEXT("StartPatrol failed: no valid patrol route points provided (Input=%d)"), NavStaticActors.Num());
        return;
    }

    for (ANavStaticBig* RouteActor : PatrolRoute)
    {
        if (!IsValid(RouteActor) || !RouteActor->PlaneRadius)
        {
            continue;
        }

 /*        RouteActor->PlaneRadius->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        RouteActor->PlaneRadius->SetGenerateOverlapEvents(true);
        RouteActor->PlaneRadius->SetCollisionResponseToAllChannels(ECR_Ignore);
        RouteActor->PlaneRadius->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap); */
    }

    bPatrolActive = true;
    bPatrolPauseActive = false;
    ActionMode = EActionMode::Patroling;
    CurrentPatrolTarget = PatrolRoute[0];

    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->TargetActor = CurrentPatrolTarget.Get();
    }
}

ANavStaticBig* AAIShipController::GetCurrentPatrolPoint() const
{
    if (!bPatrolActive || bPatrolPauseActive || ActionMode != EActionMode::Patroling || PatrolRoute.Num() == 0)
    {
        return nullptr;
    }

    AAIShipController* MutableThis = const_cast<AAIShipController*>(this);
    MutableThis->PruneInvalidPatrolHead();
    return MutableThis->CurrentPatrolTarget.Get();
}

void AAIShipController::HandlePatrolPointOverlap(UPrimitiveComponent* OtherComp, AActor* OtherActor)
{
    if (!bPatrolActive || bPatrolPauseActive || ActionMode != EActionMode::Patroling || PatrolRoute.Num() == 0 || !OtherComp || !OtherActor)
    {
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp, Verbose, TEXT("PatrolOverlap rejected (state): Active=%s Pause=%s Mode=%d Route=%d OtherComp=%s OtherActor=%s"),
            bPatrolActive ? TEXT("true") : TEXT("false"),
            bPatrolPauseActive ? TEXT("true") : TEXT("false"),
            static_cast<int32>(ActionMode),
            PatrolRoute.Num(),
            OtherComp ? *OtherComp->GetName() : TEXT("None"),
            OtherActor ? *OtherActor->GetActorNameOrLabel() : TEXT("None"));
#endif
        return;
    }

    PruneInvalidPatrolHead();
    ANavStaticBig* CurrentPoint = CurrentPatrolTarget.Get();
    if (!CurrentPoint)
    {
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp, Verbose, TEXT("PatrolOverlap rejected: CurrentPatrolTarget is null after prune"));
#endif
        return;
    }

    const bool bActorMatchesCurrentPoint = (OtherActor == CurrentPoint);
    const bool bOwnerMatchesCurrentPoint = (OtherComp->GetOwner() == CurrentPoint);
    if (!bActorMatchesCurrentPoint && !bOwnerMatchesCurrentPoint)
    {
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp, Verbose, TEXT("PatrolOverlap rejected: CurrentPoint=%s OtherActor=%s OtherCompOwner=%s"),
            *CurrentPoint->GetActorNameOrLabel(),
            OtherActor ? *OtherActor->GetActorNameOrLabel() : TEXT("None"),
            OtherComp->GetOwner() ? *OtherComp->GetOwner()->GetActorNameOrLabel() : TEXT("None"));
#endif
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("PatrolOverlap accepted: Ship reached patrol point %s"), *CurrentPoint->GetActorNameOrLabel());

    PatrolRoute.RemoveAt(0, 1, EAllowShrinking::No);
    CurrentPatrolTarget.Reset();
    PruneInvalidPatrolHead();
    if (!bPatrolActive)
    {
        return;
    }

    bPatrolPauseActive = true;
    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->TargetActor = nullptr;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ResumePatrolAfterDelay();
        return;
    }

    World->GetTimerManager().ClearTimer(PatrolResumeTimerHandle);
    if (PatrolPointDelaySeconds <= 0.0f)
    {
        ResumePatrolAfterDelay();
        return;
    }

    World->GetTimerManager().SetTimer(
        PatrolResumeTimerHandle,
        this,
        &AAIShipController::ResumePatrolAfterDelay,
        PatrolPointDelaySeconds,
        false);
}

void AAIShipController::ResumePatrolAfterDelay()
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(PatrolResumeTimerHandle);
    }

    if (ActionMode != EActionMode::Patroling)
    {
        StopPatrol(false);
        return;
    }

    bPatrolPauseActive = false;
    PruneInvalidPatrolHead();
    if (PatrolRoute.Num() > 0)
    {
        bPatrolActive = true;
        CurrentPatrolTarget = PatrolRoute[0];
    }
}

void AAIShipController::StopPatrol(bool bClearRoute)
{
    UWorld* World = GetWorld();
    if (World)
    {
        World->GetTimerManager().ClearTimer(PatrolResumeTimerHandle);
    }

    bPatrolActive = false;
    bPatrolPauseActive = false;
    CurrentPatrolTarget.Reset();
    PatrolPointDelaySeconds = 0.0f;

    if (bClearRoute)
    {
        PatrolRoute.Reset();
    }

    if (ActionMode == EActionMode::Patroling)
    {
        ActionMode = EActionMode::Idle;
    }

    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->TargetActor = nullptr;
    }
}

TArray<ANavStaticBig*> AAIShipController::CreatePatrolRoute(const TArray<ANavStaticBig*>& NavStaticActors)
{
    TArray<ANavStaticBig*> ValidActors;
    ValidActors.Reserve(NavStaticActors.Num());

    TSet<const ANavStaticBig*> UniqueActors;
    for (ANavStaticBig* Actor : NavStaticActors)
    {
        if (!IsValid(Actor) || UniqueActors.Contains(Actor))
        {
            continue;
        }

        UniqueActors.Add(Actor);
        ValidActors.Add(Actor);
    }

    PatrolRoute.Reset();
    if (ValidActors.Num() < 2)
    {
        return PatrolRoute;
    }

    for (int32 i = ValidActors.Num() - 1; i > 0; --i)
    {
        const int32 SwapIndex = FMath::RandRange(0, i);
        if (SwapIndex != i)
        {
            ValidActors.Swap(i, SwapIndex);
        }
    }

    const int32 RoutePointCount = FMath::RandRange(2, ValidActors.Num());
    TArray<ANavStaticBig*> RemainingActors;
    RemainingActors.Reserve(RoutePointCount);
    for (int32 i = 0; i < RoutePointCount; ++i)
    {
        RemainingActors.Add(ValidActors[i]);
    }

    const APawn* ControlledPawn = GetPawn();
    FVector CurrentPoint = ControlledPawn ? ControlledPawn->GetActorLocation() : FVector::ZeroVector;

    PatrolRoute.Reserve(RoutePointCount);
    while (RemainingActors.Num() > 0)
    {
        int32 ClosestIdx = 0;
        float ClosestDistSq = FVector::DistSquared(CurrentPoint, RemainingActors[0]->GetActorLocation());
        for (int32 i = 1; i < RemainingActors.Num(); ++i)
        {
            const float DistSq = FVector::DistSquared(CurrentPoint, RemainingActors[i]->GetActorLocation());
            if (DistSq < ClosestDistSq)
            {
                ClosestDistSq = DistSq;
                ClosestIdx = i;
            }
        }

        ANavStaticBig* NextActor = RemainingActors[ClosestIdx];
        PatrolRoute.Add(NextActor);
        RemainingActors.RemoveAtSwap(ClosestIdx, 1, EAllowShrinking::No);
        CurrentPoint = NextActor->GetActorLocation();
    }

    TArray<ANavStaticBig*> RouteResult;
    RouteResult.Reserve(PatrolRoute.Num());
    for (ANavStaticBig* RouteActor : PatrolRoute)
    {
        RouteResult.Add(RouteActor);
    }

    return RouteResult;
}

void AAIShipController::SetTorqueTuning(
    bool bEnableTorquePD,
    float InTorqueKpPitch,
    float InTorqueKpYaw,
    float InTorqueKdPitch,
    float InTorqueKdYaw,
    float InMaxTorquePitch,
    float InMaxTorqueYaw,
    float InMaxTorqueRoll,
    float InTorqueRollDamping)
{
    bUseTorquePD = bEnableTorquePD;
    TorqueKpPitch = InTorqueKpPitch;
    TorqueKpYaw = InTorqueKpYaw;
    TorqueKdPitch = InTorqueKdPitch;
    TorqueKdYaw = InTorqueKdYaw;
    MaxTorquePitch = InMaxTorquePitch;
    MaxTorqueYaw = InMaxTorqueYaw;
    MaxTorqueRoll = InMaxTorqueRoll;
    TorqueRollDamping = InTorqueRollDamping;
}

FVector AAIShipController::GetFocusLocation()
{
    AShip* Ship = Cast<AShip>(GetPawn());
    if (Ship && Ship->TargetActor)
    {
        return Ship->TargetActor->GetActorLocation();
    }

    return GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
}

void AAIShipController::ApplyShipRotation(FVector TargetLocation)
{
    AShip* Ship = Cast<AShip>(GetPawn());
    if (!Ship) return;

    UStaticMeshComponent* ShipBase = Ship->GetShipBase();
    if (!ShipBase) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const float DeltaTime = World->GetDeltaSeconds();
    if (DeltaTime <= 0.f) return;

    const FVector ActorLocation = Ship->GetActorLocation();
    FVector ToTargetWorld = (TargetLocation - ActorLocation);
    if (ToTargetWorld.IsNearlyZero()) return;
    ToTargetWorld.Normalize();

    const FTransform& ShipBaseTransform = ShipBase->GetComponentTransform();
    const FVector ToTargetLocal = ShipBaseTransform.InverseTransformVectorNoScale(ToTargetWorld);
    if (ToTargetLocal.IsNearlyZero()) return;

    const FVector ToTargetLocalDir = ToTargetLocal.GetSafeNormal();

    // --- Roll align error (rad) around local X axis ---
    float RollErrRad = 0.f;
    if (Ship->RollAlignMode != ERollAlignMode::Default)
    {
        FVector RollRefWorldDir = ToTargetWorld;
        if (Ship->TargetActor)
        {
            const FVector ToAlignTarget = Ship->TargetActor->GetActorLocation() - ActorLocation;
            if (!ToAlignTarget.IsNearlyZero())
            {
                RollRefWorldDir = ToAlignTarget.GetSafeNormal();
            }
        }

        const FVector RollRefLocalDir =
            ShipBaseTransform.InverseTransformVectorNoScale(RollRefWorldDir).GetSafeNormal();
        FVector DesiredUpLocal = FVector::UpVector;

        if (Ship->RollAlignMode == ERollAlignMode::BackToTarget)
            DesiredUpLocal = RollRefLocalDir;       // +Up -> target
        else if (Ship->RollAlignMode == ERollAlignMode::BellyToTarget)
            DesiredUpLocal = -RollRefLocalDir;      // -Up -> target

        // Roll is rotation around local X, so remove X component
        DesiredUpLocal.X = 0.f;
        if (DesiredUpLocal.IsNearlyZero())
            DesiredUpLocal = FVector(0.f, 0.f, 1.f);
        DesiredUpLocal.Normalize();

        const FVector CurrentUpLocal(0.f, 0.f, 1.f);

        // Signed angle from CurrentUp to DesiredUp around Forward (local X axis)
        const float CrossX = FVector::CrossProduct(CurrentUpLocal, DesiredUpLocal).X;
        const float DotUD  = FVector::DotProduct(CurrentUpLocal, DesiredUpLocal);
        RollErrRad = FMath::Atan2(CrossX, DotUD); // [-pi,pi]

#if !UE_BUILD_SHIPPING
        if (Ship->bDebugSteering && World)
        {
            RollAlignDebugAccumulator += DeltaTime;
            if (RollAlignDebugAccumulator >= 0.5f)
            {
                RollAlignDebugAccumulator = 0.0f;
                const TCHAR* AlignSource = Ship->TargetActor ? TEXT("Target") : TEXT("Steering");
                UE_LOG(LogTemp, Log, TEXT("Ship %s RollAlign[%s] ErrDeg=%.2f"),
                    *Ship->GetActorNameOrLabel(),
                    AlignSource,
                    FMath::RadiansToDegrees(RollErrRad));
            }
            const FVector DrawStart = ActorLocation;
            const FVector DrawEnd = ActorLocation + RollRefWorldDir * 800.0f;
            DrawDebugDirectionalArrow(World, DrawStart, DrawEnd, 120.0f, FColor::Purple, false, 0.0f, 0, 2.0f);
        }
#endif
    }

    // Check if target is behind the ship
    const bool bTargetBehind = (ToTargetLocalDir.X < 0.f);

    float PitchErr = -FMath::RadiansToDegrees(
        FMath::Atan2(ToTargetLocalDir.Z, ToTargetLocalDir.X)
    );
    float YawErr = FMath::RadiansToDegrees(
        FMath::Atan2(ToTargetLocalDir.Y, ToTargetLocalDir.X)
    );

    // Handle target behind to prevent 179° ↔ -179° thrash
    if (bTargetBehind)
    {
        // Compute preferred yaw sign to maintain consistent turning direction
        float PreferredYawSign = 0.f;
        if (FMath::Abs(FilteredYawRate) > 1.f) 
            PreferredYawSign = FMath::Sign(FilteredYawRate);
        else 
            PreferredYawSign = (YawErr >= 0.f) ? 1.f : -1.f;

        // Prevent sudden yaw sign flip near 180 deg
        YawErr = FMath::Abs(YawErr) * PreferredYawSign;

        // Reduce pitch when behind to avoid weird pitch flips
        PitchErr *= 0.5f;
    }

    // Add deadzone (you already have AimDeadZoneDeg in header)
    float PitchErrDz = (FMath::Abs(PitchErr) < AimDeadZoneDeg) ? 0.f : PitchErr;
    float YawErrDz   = (FMath::Abs(YawErr)   < AimDeadZoneDeg) ? 0.f : YawErr;

    // --- Torque PD branch ---
    if (bUseTorquePD)
    {
        // Current angular velocity in local (deg/s)
        const FVector CurAngVelWorldDeg = ShipBase->GetPhysicsAngularVelocityInDegrees();
        const FVector CurAngVelLocalDeg = ShipBaseTransform.InverseTransformVectorNoScale(CurAngVelWorldDeg);

        // Convert to rad/s
        const FVector CurAngVelLocalRad = CurAngVelLocalDeg * (PI / 180.f);

        // Convert error to radians
        const float PitchErrRad = FMath::DegreesToRadians(PitchErrDz);
        const float YawErrRad   = FMath::DegreesToRadians(YawErrDz);

        // Safety brake: when errors are basically zero, prevent residual spin
        const bool bAlmostAimed =
            (FMath::Abs(PitchErrDz) < (AimDeadZoneDeg * 2.f)) &&
            (FMath::Abs(YawErrDz)   < (AimDeadZoneDeg * 2.f));

        // Use component inertia tensor (kg*cm^2)
        // In UE, GetInertiaTensor returns in *component local* principal axes.
        const FVector Inertia = ShipBase->GetInertiaTensor();
        const float Ix = FMath::Max(Inertia.X, 1.f);
        const float Iy = FMath::Max(Inertia.Y, 1.f);
        const float Iz = FMath::Max(Inertia.Z, 1.f);

        // Desired angular acceleration (rad/s^2) per local axis:
        // Pitch = local Y, Yaw = local Z
        float AlphaPitch, AlphaYaw;
        
        if (bAlmostAimed)
        {
            // Increase damping temporarily near lock
            AlphaPitch = - (TorqueKdPitch * 1.5f) * CurAngVelLocalRad.Y;
            AlphaYaw   = - (TorqueKdYaw   * 1.5f) * CurAngVelLocalRad.Z;
        }
        else
        {
            // Normal PD control
            AlphaPitch = (TorqueKpPitch * PitchErrRad) - (TorqueKdPitch * CurAngVelLocalRad.Y);
            AlphaYaw   = (TorqueKpYaw   * YawErrRad)   - (TorqueKdYaw   * CurAngVelLocalRad.Z);
        }

        // Convert to torque τ = I * α (N*cm)
        float TorquePitch = Iy * AlphaPitch;
        float TorqueYaw   = Iz * AlphaYaw;

        float TorqueRoll = 0.f;
        if (Ship->RollAlignMode == ERollAlignMode::Default)
        {
            float RollLevelErrRad = 0.f;
            bool bApplyForwardRollLevel = false;
            if (bEnableForwardRollLevel)
            {
                const FVector CurVelWorld = ShipBase->GetPhysicsLinearVelocity();
                const FVector CurVelLocal = ShipBaseTransform.InverseTransformVectorNoScale(CurVelWorld);
                bApplyForwardRollLevel = CurVelLocal.X > ForwardRollLevelSpeedThreshold;
            }

            if (bApplyForwardRollLevel)
            {
                const FVector WorldUpLocal = ShipBaseTransform.InverseTransformVectorNoScale(FVector::UpVector);
                FVector DesiredUpLocal = WorldUpLocal;
                DesiredUpLocal.X = 0.f;
                if (DesiredUpLocal.IsNearlyZero())
                {
                    DesiredUpLocal = FVector(0.f, 0.f, 1.f);
                }
                DesiredUpLocal.Normalize();

                const FVector CurrentUpLocal(0.f, 0.f, 1.f);
                const float CrossX = FVector::CrossProduct(CurrentUpLocal, DesiredUpLocal).X;
                const float DotUD = FVector::DotProduct(CurrentUpLocal, DesiredUpLocal);
                RollLevelErrRad = FMath::Atan2(CrossX, DotUD);

                const float RollKp = Ship->RollAlignKp * ForwardRollLevelGainScale;
                const float RollKd = Ship->RollAlignKd * ForwardRollLevelGainScale;
                const float AlphaRoll = (RollKp * RollLevelErrRad) - (RollKd * CurAngVelLocalRad.X);
                TorqueRoll = Ix * AlphaRoll;
            }
            else
            {
                // Old behavior: damping only
                TorqueRoll = -Ix * (TorqueRollDamping * CurAngVelLocalRad.X);
            }
        }
        else
        {
            // Roll align PD (rad)
            const float AlphaRoll =
                (Ship->RollAlignKp * RollErrRad) - (Ship->RollAlignKd * CurAngVelLocalRad.X);

            TorqueRoll = Ix * AlphaRoll;
        }

        // Clamp torques
        TorquePitch = FMath::Clamp(TorquePitch, -MaxTorquePitch, MaxTorquePitch);
        TorqueYaw   = FMath::Clamp(TorqueYaw,   -MaxTorqueYaw,   MaxTorqueYaw);
        TorqueRoll  = FMath::Clamp(TorqueRoll,  -MaxTorqueRoll,  MaxTorqueRoll);

        // Local torque vector (X=roll, Y=pitch, Z=yaw) in N*cm
        const FVector TorqueLocal(TorqueRoll, TorquePitch, TorqueYaw);

        // Transform to world and apply in radians
        const FVector TorqueWorld = ShipBaseTransform.TransformVectorNoScale(TorqueLocal);
        ShipBase->AddTorqueInRadians(TorqueWorld, NAME_None, true);

        return; // IMPORTANT: do not also SetPhysicsAngularVelocity
    }

    // --- Existing angular-velocity servo branch (unchanged) ---
    const float DesiredPitchRate = FMath::Clamp(PitchErrDz * 0.5f, -Ship->MaxPitchSpeed, Ship->MaxPitchSpeed);
    const float DesiredYawRate   = FMath::Clamp(YawErrDz   * 0.55f, -Ship->MaxYawSpeed,  Ship->MaxYawSpeed);

    float DesiredRollRate = 0.f;
    if (Ship->RollAlignMode == ERollAlignMode::Default)
    {
        float RollLevelErrRad = 0.f;
        bool bApplyForwardRollLevel = false;
        if (bEnableForwardRollLevel)
        {
            const FVector CurVelWorld = ShipBase->GetPhysicsLinearVelocity();
            const FVector CurVelLocal = ShipBaseTransform.InverseTransformVectorNoScale(CurVelWorld);
            bApplyForwardRollLevel = CurVelLocal.X > ForwardRollLevelSpeedThreshold;
        }

        if (bApplyForwardRollLevel)
        {
            const FVector WorldUpLocal = ShipBaseTransform.InverseTransformVectorNoScale(FVector::UpVector);
            FVector DesiredUpLocal = WorldUpLocal;
            DesiredUpLocal.X = 0.f;
            if (DesiredUpLocal.IsNearlyZero())
            {
                DesiredUpLocal = FVector(0.f, 0.f, 1.f);
            }
            DesiredUpLocal.Normalize();

            const FVector CurrentUpLocal(0.f, 0.f, 1.f);
            const float CrossX = FVector::CrossProduct(CurrentUpLocal, DesiredUpLocal).X;
            const float DotUD = FVector::DotProduct(CurrentUpLocal, DesiredUpLocal);
            RollLevelErrRad = FMath::Atan2(CrossX, DotUD);

            const float RollErrDeg = FMath::RadiansToDegrees(RollLevelErrRad);
            DesiredRollRate = FMath::Clamp(
                RollErrDeg * 0.5f * ForwardRollLevelGainScale,
                -Ship->MaxRollSpeed,
                Ship->MaxRollSpeed);
        }
    }
    else
    {
        const float RollErrDeg = FMath::RadiansToDegrees(RollErrRad);
        DesiredRollRate = FMath::Clamp(RollErrDeg * 0.5f, -Ship->MaxRollSpeed, Ship->MaxRollSpeed);
    }

    const FVector TargetAngVelLocal(DesiredRollRate, DesiredPitchRate, DesiredYawRate);
    Ship->ApplyRotationServo(TargetAngVelLocal, DeltaTime);
}

void AAIShipController::HandleSafetyMarginCheck()
{
    AShip* Ship = Cast<AShip>(GetPawn());
    if (!Ship || !Ship->ShipRadius)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleActor.Reset();
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    if (World->GetTimeSeconds() < SafetyMarginSuppressUntilTime)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleComp.Reset();
        CurrentNavObstacleActor.Reset();
        return;
    }

	UPrimitiveComponent* ObstacleComp = CurrentObstacleComp.Get();
    FVector ClosestPoint = FVector::ZeroVector;
	if (ObstacleComp && (ObstacleComp->GetOwner() == Ship))
	{
		ObstacleComp = nullptr;
		CurrentObstacleComp.Reset();
		CurrentNavObstacleActor.Reset();
		bInsideSafetyMargin = false;
	}
    const FVector ShipLocation = Ship->GetActorLocation();
    const bool bHasCurrentPoint = ObstacleComp
        && ObstacleComp->GetClosestPointOnCollision(ShipLocation, ClosestPoint);
    if (!ObstacleComp || !bHasCurrentPoint)
    {
        const float ShipRadiusValue = Ship->ShipRadius->GetScaledSphereRadius();
        const float QueryRadius = ShipRadiusValue + NavSafetyMargin;
        if (QueryRadius > KINDA_SMALL_NUMBER)
        {
            FCollisionObjectQueryParams ObjectParams;
            ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
            ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
            ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
            FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SafetyMarginQuery), false);
            QueryParams.AddIgnoredActor(Ship);

            TArray<FOverlapResult> Overlaps;
            World->OverlapMultiByObjectType(
                Overlaps,
                ShipLocation,
                FQuat::Identity,
                ObjectParams,
                FCollisionShape::MakeSphere(QueryRadius),
                QueryParams);

            float BestDistanceSq = TNumericLimits<float>::Max();
            UPrimitiveComponent* BestComp = nullptr;
            FVector BestPoint = FVector::ZeroVector;
            for (const FOverlapResult& Overlap : Overlaps)
            {
                UPrimitiveComponent* OverlapComp = Overlap.GetComponent();
                if (!OverlapComp)
                {
                    continue;
                }
                if (OverlapComp == Ship->ShipRadius || OverlapComp == Ship->GetShipBase()
                    || OverlapComp->GetOwner() == Ship)
                {
                    continue;
                }

                const bool bBlocksStatic = OverlapComp->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block;
                const bool bBlocksDynamic = OverlapComp->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block;
                const bool bBlocksPhysics = OverlapComp->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Block;
                if (!bBlocksStatic && !bBlocksDynamic && !bBlocksPhysics)
                {
                    continue;
                }

                FVector OverlapClosestPoint = FVector::ZeroVector;
                if (!OverlapComp->GetClosestPointOnCollision(ShipLocation, OverlapClosestPoint))
                {
                    continue;
                }

                const float DistanceSq = FVector::DistSquared(ShipLocation, OverlapClosestPoint);
                if (DistanceSq < BestDistanceSq)
                {
                    BestDistanceSq = DistanceSq;
                    BestComp = OverlapComp;
                    BestPoint = OverlapClosestPoint;
                }
            }

            if (BestComp)
            {
                SetCurrentObstacleComp(BestComp);
                if (bDebugSafetyMargin)
                {
                    DrawDebugPoint(World, BestPoint, 10.0f, FColor::Red, false, 0.0f, 0);
                }
            }
        }
    }

    UpdateInsideSafetyMargin(Ship->ShipRadius);
}

void AAIShipController::SuppressSafetyMargin(float DurationSeconds)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();
    const float Duration = FMath::Max(0.0f, DurationSeconds);
    SafetyMarginSuppressUntilTime = FMath::Max(SafetyMarginSuppressUntilTime, Now + Duration);
    bInsideSafetyMargin = false;
    CurrentNavObstacleComp.Reset();
    CurrentNavObstacleActor.Reset();
}

bool AAIShipController::UpdateInsideSafetyMargin(USphereComponent* ShipRadius)
{
    if (!ShipRadius)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleActor.Reset();
        return false;
    }

    UPrimitiveComponent* ObstacleComp = nullptr;
    if (bInsideSafetyMargin && CurrentNavObstacleComp.IsValid())
    {
        ObstacleComp = CurrentNavObstacleComp.Get();
    }
    else
    {
        ObstacleComp = CurrentObstacleComp.Get();
    }
    if (!ObstacleComp)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleActor.Reset();
        return false;
    }

    const APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleActor.Reset();
        return false;
    }
    if (ObstacleComp->GetOwner() == ControlledPawn)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleComp.Reset();
        CurrentNavObstacleActor.Reset();
        return false;
    }

    const FVector PawnLocation = ControlledPawn->GetActorLocation();

    FVector ClosestPoint = FVector::ZeroVector;
    const bool bHasPoint = ObstacleComp->GetClosestPointOnCollision(
        PawnLocation,
        ClosestPoint);
    if (!bHasPoint)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleActor.Reset();
        return false;
    }

    const float DistanceSq = FVector::DistSquared(PawnLocation, ClosestPoint);
    const float ShipRadiusValue = ShipRadius->GetScaledSphereRadius();
    const float EnterMargin = NavSafetyMargin + ShipRadiusValue;
    const float ExitMargin = EnterMargin + (ShipRadiusValue * SafetyExitHysteresisMultiplier);
    const float EnterMarginSq = FMath::Square(EnterMargin);
    const float ExitMarginSq = FMath::Square(ExitMargin);
    if (!bInsideSafetyMargin)
    {
        bInsideSafetyMargin = DistanceSq < EnterMarginSq;
        if (bInsideSafetyMargin)
        {
            CurrentNavObstacleComp = ObstacleComp;
            CurrentNavObstacleActor = ObstacleComp->GetOwner();
        }
    }
    else
    {
        if (DistanceSq > ExitMarginSq)
        {
            bInsideSafetyMargin = false;
            CurrentNavObstacleComp.Reset();
            CurrentNavObstacleActor.Reset();
        }
        else
        {
            bInsideSafetyMargin = true;
            CurrentNavObstacleComp = ObstacleComp;
            CurrentNavObstacleActor = ObstacleComp->GetOwner();
        }
    }

    return bInsideSafetyMargin;
}

FVector AAIShipController::ComputeEscapeTarget(const FVector& ShipLocation, USphereComponent* ShipRadius) const
{
    if (!ShipRadius)
    {
        return ShipLocation;
    }

    const APawn* ControlledPawn = GetPawn();
    const bool bLogSafety = bDebugSafetyMargin;
    const FString ShipLabel = ControlledPawn ? ControlledPawn->GetActorNameOrLabel() : TEXT("None");

    const UPrimitiveComponent* ObstacleComp =
        (bInsideSafetyMargin && CurrentNavObstacleComp.IsValid())
            ? CurrentNavObstacleComp.Get()
            : CurrentObstacleComp.Get();
    const AActor* ObstacleActor = CurrentNavObstacleActor.Get();
    if (!ObstacleComp && !ObstacleActor)
    {
        if (bLogSafety)
        {
            UE_LOG(LogTemp, Verbose, TEXT("Ship %s EscapeTarget invalid: no obstacle"), *ShipLabel);
        }
        return ShipLocation;
    }
    if (ControlledPawn && ((ObstacleComp && ObstacleComp->GetOwner() == ControlledPawn)
        || ObstacleActor == ControlledPawn))
    {
        if (bLogSafety)
        {
            UE_LOG(LogTemp, Verbose, TEXT("Ship %s EscapeTarget invalid: obstacle is self"), *ShipLabel);
        }
        return ShipLocation;
    }

    FVector ContactPoint = FVector::ZeroVector;
    const bool bHasPoint = ObstacleComp
        && ObstacleComp->GetClosestPointOnCollision(ShipLocation, ContactPoint);
    if (!bHasPoint && ObstacleComp && !ObstacleActor)
    {
        ObstacleActor = ObstacleComp->GetOwner();
    }
    if (!bHasPoint && !ObstacleActor)
    {
        if (bLogSafety)
        {
            UE_LOG(LogTemp, Verbose, TEXT("Ship %s EscapeTarget invalid: no contact point (ObstacleComp=%s)"),
                *ShipLabel,
                ObstacleComp ? *ObstacleComp->GetName() : TEXT("None"));
        }
        return ShipLocation;
    }

    FVector Normal = bHasPoint
        ? (ShipLocation - ContactPoint).GetSafeNormal()
        : FVector::ZeroVector;
    if (Normal.IsNearlyZero() && ObstacleActor)
    {
        Normal = (ShipLocation - ObstacleActor->GetActorLocation()).GetSafeNormal();
    }

    if (Normal.IsNearlyZero())
    {
        if (bLogSafety)
        {
            UE_LOG(LogTemp, Verbose, TEXT("Ship %s EscapeTarget invalid: zero normal (ObstacleComp=%s ObstacleActor=%s)"),
                *ShipLabel,
                ObstacleComp ? *ObstacleComp->GetName() : TEXT("None"),
                ObstacleActor ? *ObstacleActor->GetActorNameOrLabel() : TEXT("None"));
        }
        return ShipLocation;
    }

    const float ShipRadiusValue = ShipRadius->GetScaledSphereRadius();
    const float EscapeDistance = (ShipRadiusValue * 2.0f) + NavSafetyMargin;
    FVector EscapeTarget = ShipLocation + Normal * EscapeDistance;

    const AShip* Ship = ControlledPawn ? Cast<AShip>(ControlledPawn) : nullptr;
    const bool bHasTargetActor = Ship && Ship->TargetActor;
    if (ControlledPawn && bHasTargetActor)
    {
        const FVector Forward = ControlledPawn->GetActorForwardVector();
        const FVector Tangent = (Forward - FVector::DotProduct(Forward, Normal) * Normal).GetSafeNormal();
        if (!Tangent.IsNearlyZero())
        {
            EscapeTarget += Tangent * ShipRadiusValue;
        }
    }

    return EscapeTarget;
}

void AAIShipController::HandleStuckCheck()
{
	const bool bWasUnstucking = bIsUnstucking;
	AShip* Ship = Cast<AShip>(GetPawn());
	if (!Ship)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UStaticMeshComponent* ShipBase = Ship->GetShipBase();
	if (!ShipBase)
	{
		return;
	}

	UPrimitiveComponent* ObstacleComp = CurrentObstacleComp.Get();
	if (!ObstacleComp && Ship->ShipRadius)
	{
		const float ShipRadiusValue = Ship->ShipRadius->GetScaledSphereRadius();
		const float QueryRadius = ShipRadiusValue + NavSafetyMargin;
		if (QueryRadius > KINDA_SMALL_NUMBER)
		{
			FCollisionObjectQueryParams ObjectParams;
			ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
			ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
			ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UnstuckReacquire), false);
			QueryParams.AddIgnoredActor(Ship);

			TArray<FOverlapResult> Overlaps;
			World->OverlapMultiByObjectType(
				Overlaps,
				Ship->GetActorLocation(),
				FQuat::Identity,
				ObjectParams,
				FCollisionShape::MakeSphere(QueryRadius),
				QueryParams);

			float BestDistanceSq = TNumericLimits<float>::Max();
			UPrimitiveComponent* BestComp = nullptr;
			for (const FOverlapResult& Overlap : Overlaps)
			{
				UPrimitiveComponent* OverlapComp = Overlap.GetComponent();
				if (!OverlapComp)
				{
					continue;
				}
				if (OverlapComp == Ship->ShipRadius || OverlapComp == Ship->GetShipBase()
					|| OverlapComp->GetOwner() == Ship)
				{
					continue;
				}

				const bool bBlocksStatic = OverlapComp->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block;
				const bool bBlocksDynamic = OverlapComp->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block;
				const bool bBlocksPhysics = OverlapComp->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Block;
				if (!bBlocksStatic && !bBlocksDynamic && !bBlocksPhysics)
				{
					continue;
				}

				FVector ClosestPoint = FVector::ZeroVector;
				if (!OverlapComp->GetClosestPointOnCollision(Ship->GetActorLocation(), ClosestPoint))
				{
					continue;
				}

				const float DistanceSq = FVector::DistSquared(Ship->GetActorLocation(), ClosestPoint);
				if (DistanceSq < BestDistanceSq)
				{
					BestDistanceSq = DistanceSq;
					BestComp = OverlapComp;
				}
			}

			if (BestComp)
			{
				SetCurrentObstacleComp(BestComp);
				ObstacleComp = BestComp;
			}
		}
	}

	const FVector Velocity = ShipBase->GetPhysicsLinearVelocity();
	const float Speed = Velocity.Size();
	const FVector GoalLocation = GetFocusLocation();
	const float DistanceToGoal = FVector::Dist(Ship->GetActorLocation(), GoalLocation);

	if (Speed < MinStuckSpeed && DistanceToGoal >= PrevDistanceToGoal)
	{
		StuckAccumulatedTime += StuckCheckInterval;
	}
	else
	{
		StuckAccumulatedTime = 0.0f;
	}

	PrevDistanceToGoal = DistanceToGoal;
#if !UE_BUILD_SHIPPING
	if (bDebugUnstuck)
	{
		const APawn* ControlledPawn = GetPawn();
		UE_LOG(LogTemp, Verbose,
			TEXT("Ship %s UnstuckCheck Speed=%.1f Dist=%.1f StuckTime=%.2f Obstacle=%s"),
			ControlledPawn ? *ControlledPawn->GetActorNameOrLabel() : TEXT("None"),
			Speed,
			DistanceToGoal,
			StuckAccumulatedTime,
			CurrentObstacleComp.IsValid() ? *CurrentObstacleComp->GetName() : TEXT("None"));
	}
#endif
	if (!bIsUnstucking)
	{
		bIsUnstucking = (StuckAccumulatedTime >= StuckTimeThreshold);
		if (bIsUnstucking)
		{
			const float CurrentTime = World->GetTimeSeconds();
			UnstuckEndTime = CurrentTime + UnstuckDuration;
			LastUnstuckForceTime = 0.0f;

#if !UE_BUILD_SHIPPING
			if (bDebugUnstuck)
			{
				const APawn* ControlledPawn = GetPawn();
				UE_LOG(LogTemp, Warning, TEXT("Ship %s Unstuck start"),
					ControlledPawn ? *ControlledPawn->GetActorNameOrLabel() : TEXT("None"));
			}
#endif
		}
	}

	if (!bIsUnstucking)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (CurrentTime >= UnstuckEndTime
		|| Speed > (MinStuckSpeed * 1.5f)
		|| !CurrentObstacleComp.IsValid())
	{
		if (bIsUnstucking)
		{
#if !UE_BUILD_SHIPPING
			if (bDebugUnstuck)
			{
				const APawn* ControlledPawn = GetPawn();
				UE_LOG(LogTemp, Warning, TEXT("Ship %s Unstuck stop (%s)"),
					ControlledPawn ? *ControlledPawn->GetActorNameOrLabel() : TEXT("None"),
					CurrentObstacleComp.IsValid() ? TEXT("Recovered") : TEXT("LostObstacle"));
			}
#endif
		}
		bIsUnstucking = false;
		StuckAccumulatedTime = 0.0f;
		PrevDistanceToGoal = DistanceToGoal;
		UnstuckEndTime = 0.0f;
		LastUnstuckForceTime = 0.0f;
		return;
	}

	if (CurrentTime < LastUnstuckForceTime + UnstuckForceInterval)
	{
		return;
	}

	const FVector ContactPoint = GetCurrentObstacleContactPoint();
	if (ContactPoint.IsNearlyZero())
	{
		return;
	}

	const FVector ShipLocation = Ship->GetActorLocation();
	const FVector PushDir = (ShipLocation - ContactPoint).GetSafeNormal();
	if (PushDir.IsNearlyZero())
	{
		return;
	}

	const float ShipRadius = Ship->ShipRadius ? Ship->ShipRadius->GetScaledSphereRadius() : 0.0f;
	const float EffectiveRadius = ShipRadius + NavSafetyMargin;
	if (EffectiveRadius <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float Distance = FVector::Dist(ShipLocation, ContactPoint);
	float PenetrationAlpha = FMath::Clamp(1.0f - (Distance / EffectiveRadius), 0.0f, 1.0f);
	if (PenetrationAlpha <= 0.0f && Distance <= (EffectiveRadius * 1.2f))
	{
		PenetrationAlpha = 0.2f;
	}
	if (PenetrationAlpha <= 0.0f)
	{
		return;
	}

	const FVector Force = PushDir * UnstuckForceStrength * PenetrationAlpha;
	ShipBase->AddForceAtLocation(Force, ContactPoint);
	LastUnstuckForceTime = CurrentTime;

#if !UE_BUILD_SHIPPING
	if (!bWasUnstucking && bIsUnstucking && bDebugUnstuck)
	{
		const APawn* ControlledPawn = GetPawn();
		UE_LOG(LogTemp, Warning, TEXT("Ship %s Unstuck start"),
			ControlledPawn ? *ControlledPawn->GetActorNameOrLabel() : TEXT("None"));
	}
	if (bDebugUnstuck)
	{
		const APawn* ControlledPawn = GetPawn();
		UE_LOG(LogTemp, Verbose, TEXT("Ship %s Unstuck force %.1f Pen=%.2f"),
			ControlledPawn ? *ControlledPawn->GetActorNameOrLabel() : TEXT("None"),
			Force.Size(),
			PenetrationAlpha);
	}
#endif
}

void AAIShipController::SetCurrentObstacleComp(UPrimitiveComponent* ObstacleComp)
{
    CurrentObstacleComp = ObstacleComp;
}

UPrimitiveComponent* AAIShipController::GetCurrentObstacleComp() const
{
    return CurrentObstacleComp.Get();
}

FVector AAIShipController::GetCurrentObstacleContactPoint() const
{
    const UPrimitiveComponent* ObstacleComp = CurrentObstacleComp.Get();
    if (!ObstacleComp)
    {
        return FVector::ZeroVector;
    }

    const APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn)
    {
        return FVector::ZeroVector;
    }

    FVector ClosestPoint = FVector::ZeroVector;
    const bool bHasPoint = ObstacleComp->GetClosestPointOnCollision(
        ControlledPawn->GetActorLocation(),
        ClosestPoint);
    return bHasPoint ? ClosestPoint : FVector::ZeroVector;
}

bool AAIShipController::IsUnstucking() const
{
    return bIsUnstucking;
}
