#include "AIShipController.h"
#include "Ship.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

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
