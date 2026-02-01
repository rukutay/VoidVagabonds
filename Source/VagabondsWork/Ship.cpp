#include "Ship.h"
#include "AIShipController.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "ShipNavComponent.h"

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
    const FVector Goal = GetGoalLocation();
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
    }
    else if (ShipNav && (!ShipController || !ShipController->IsUnstucking()))
    {
        ShipNav->TickNav(DeltaTime, Goal, ShipRadiusCm);
        SteeringTarget = ShipNav->GetNavTarget(Goal);
    }

    ApplySteeringForce(SteeringTarget, DeltaTime);

    // Rotation
    // NAVIGATION_TODO_REMOVE
    if (ShipController)
    {
        ShipController->ApplyShipRotation(SteeringTarget);
    }

#if !UE_BUILD_SHIPPING
    if (ShipNav)
    {
        DrawDebugPoint(GetWorld(), SteeringTarget, 16.0f, FColor::Cyan, false, 0.0f, 0);
    }
#endif
}

void AShip::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
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

    const bool bBlocksStatic = OtherComp->GetCollisionResponseToChannel(ECC_WorldStatic) == ECR_Block;
    const bool bBlocksDynamic = OtherComp->GetCollisionResponseToChannel(ECC_WorldDynamic) == ECR_Block;
    const bool bBlocksPhysics = OtherComp->GetCollisionResponseToChannel(ECC_PhysicsBody) == ECR_Block;
    if (!bBlocksStatic && !bBlocksDynamic && !bBlocksPhysics)
    {
        return;
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

#if !UE_BUILD_SHIPPING
    DrawDebugLine(GetWorld(), ActorLocation, TargetLocation, FColor::Green, false, 0.f, 0, 2.f);
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
#endif
}

bool AShip::EnsureShipController()
{
    if (!ShipController)
    {
        ShipController = Cast<AAIShipController>(GetController());
    }
    return ShipController != nullptr;
}
