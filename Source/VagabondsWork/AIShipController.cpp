#include "AIShipController.h"
#include "Ship.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

void AAIShipController::BeginPlay()
{
    Super::BeginPlay();

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

FVector AAIShipController::GetFocusLocation()
{
    AShip* Ship = Cast<AShip>(GetPawn());
    if (Ship && Ship->TargetActor && !bInsideSafetyMargin)
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

    UpdateInsideSafetyMargin(Ship->ShipRadius);
}

bool AAIShipController::UpdateInsideSafetyMargin(USphereComponent* ShipRadius)
{
    if (!ShipRadius)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleActor.Reset();
        return false;
    }

    UPrimitiveComponent* ObstacleComp = CurrentObstacleComp.Get();
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

    FVector ClosestPoint = FVector::ZeroVector;
    const bool bHasPoint = ObstacleComp->GetClosestPointOnCollision(
        ControlledPawn->GetActorLocation(),
        ClosestPoint);
    if (!bHasPoint)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleActor.Reset();
        return false;
    }

    const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), ClosestPoint);
    const float ShipRadiusValue = ShipRadius->GetScaledSphereRadius();
    const float Margin = NavSafetyMargin + ShipRadiusValue;
    const float ExitMargin = Margin + (ShipRadiusValue * SafetyExitHysteresisMultiplier);
    bInsideSafetyMargin = Distance < Margin;
    if (bInsideSafetyMargin)
    {
        CurrentNavObstacleComp = ObstacleComp;
        CurrentNavObstacleActor = ObstacleComp->GetOwner();
    }
    else if (Distance > ExitMargin)
    {
        bInsideSafetyMargin = false;
        CurrentNavObstacleComp.Reset();
        CurrentNavObstacleActor.Reset();
    }

    return bInsideSafetyMargin;
}

FVector AAIShipController::ComputeEscapeTarget(const FVector& ShipLocation, USphereComponent* ShipRadius) const
{
    if (!ShipRadius)
    {
        return ShipLocation;
    }

    const UPrimitiveComponent* ObstacleComp = CurrentObstacleComp.Get();
    if (!ObstacleComp)
    {
        return ShipLocation;
    }

    FVector ContactPoint = FVector::ZeroVector;
    const bool bHasPoint = ObstacleComp->GetClosestPointOnCollision(ShipLocation, ContactPoint);
    if (!bHasPoint)
    {
        return ShipLocation;
    }

    FVector Normal = (ShipLocation - ContactPoint).GetSafeNormal();
    if (Normal.IsNearlyZero())
    {
        if (const AActor* ObstacleActor = ObstacleComp->GetOwner())
        {
            Normal = (ShipLocation - ObstacleActor->GetActorLocation()).GetSafeNormal();
        }
    }

    if (Normal.IsNearlyZero())
    {
        return ShipLocation;
    }

    const float ShipRadiusValue = ShipRadius->GetScaledSphereRadius();
    const float EscapeDistance = (ShipRadiusValue * 2.0f) + NavSafetyMargin;
    FVector EscapeTarget = ShipLocation + Normal * EscapeDistance;

    if (const APawn* ControlledPawn = GetPawn())
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
    AShip* Ship = Cast<AShip>(GetPawn());
    if (!Ship)
    {
        return;
    }

    UStaticMeshComponent* ShipBase = Ship->GetShipBase();
    if (!ShipBase)
    {
        return;
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
    if (!bIsUnstucking)
    {
        bIsUnstucking = (StuckAccumulatedTime >= StuckTimeThreshold);
        if (bIsUnstucking)
        {
            const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
            UnstuckEndTime = CurrentTime + UnstuckDuration;
            LastUnstuckForceTime = 0.0f;
        }
    }

    if (!bIsUnstucking)
    {
        return;
    }

    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    if (CurrentTime >= UnstuckEndTime
        || Speed > (MinStuckSpeed * 1.5f)
        || !CurrentObstacleComp.IsValid())
    {
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
    if (ShipRadius <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float Distance = FVector::Dist(ShipLocation, ContactPoint);
    const float PenetrationAlpha = FMath::Clamp(1.0f - (Distance / ShipRadius), 0.0f, 1.0f);
    if (PenetrationAlpha <= 0.0f)
    {
        return;
    }

    const FVector Force = PushDir * UnstuckForceStrength * PenetrationAlpha;
    ShipBase->AddForceAtLocation(Force, ContactPoint);
    LastUnstuckForceTime = CurrentTime;
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
