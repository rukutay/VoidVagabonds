#include "Ship.h"
#include "AIShipController.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "ShipNavComponent.h"
#include "ShipVitalityComponent.h"
#include "MarkerComponent.h"
#include "NavStaticBig.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SpringArmComponent.h"

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
    ShipBase->SetGenerateOverlapEvents(true);

    ShipRadius = CreateDefaultSubobject<USphereComponent>(TEXT("ShipRadius"));
    ShipRadius->SetupAttachment(ShipBase);

    InternalScanerRadius = CreateDefaultSubobject<USphereComponent>(TEXT("InternalScanerRadius"));
    InternalScanerRadius->SetupAttachment(ShipBase);
    InternalScanerRadius->SetSphereRadius(6400000.f);

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(ShipBase);
    CameraBoom->TargetArmLength = CameraBoomLength;
    CameraBoom->bUsePawnControlRotation = true;

    ShipCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ShipCamera"));
    ShipCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    ShipCamera->bUsePawnControlRotation = false;

    ShipNav = CreateDefaultSubobject<UShipNavComponent>(TEXT("ShipNav"));

    Vitality = CreateDefaultSubobject<UShipVitalityComponent>(TEXT("Vitality"));

    MarkerComponent = CreateDefaultSubobject<UMarkerComponent>(TEXT("MarkerComponent"));
    MarkerComponent->MarkerType = EMarkerType::Ship;

    // NAVIGATION_TODO_REMOVE
    ShipRadius->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ShipRadius->SetCollisionProfileName(TEXT("Pawn"));
    ShipRadius->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Overlap);
    ShipRadius->SetSphereRadius(300.f);

    ShipRadius->OnComponentBeginOverlap.AddDynamic(this, &AShip::HandleShipRadiusBeginOverlap);
    ShipRadius->OnComponentEndOverlap.AddDynamic(this, &AShip::HandleShipRadiusEndOverlap);
}

float AShip::GetCameraBoomLength() const
{
    return CameraBoom ? CameraBoom->TargetArmLength : CameraBoomLength;
}

FTransform AShip::GetShipCameraTransform() const
{
    if (ShipCamera)
    {
        return ShipCamera->GetComponentTransform();
    }

    return GetActorTransform();
}

void AShip::BeginPlay()
{
    Super::BeginPlay();

    if (ShipBase)
    {
        ShipBase->SetGenerateOverlapEvents(true);
        ShipBase->OnComponentBeginOverlap.RemoveDynamic(this, &AShip::HandlePatrolPointBeginOverlap);
        ShipBase->OnComponentBeginOverlap.AddDynamic(this, &AShip::HandlePatrolPointBeginOverlap);
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp, Log, TEXT("Ship %s Patrol bind on ShipBase (%s)"),
            *GetActorNameOrLabel(),
            *ShipBase->GetName());
#endif
    }

    if (ShipRadius && !ShipRadius->OnComponentBeginOverlap.IsAlreadyBound(this, &AShip::HandlePatrolPointBeginOverlap))
    {
        ShipRadius->OnComponentBeginOverlap.AddDynamic(this, &AShip::HandlePatrolPointBeginOverlap);
#if !UE_BUILD_SHIPPING
        UE_LOG(LogTemp, Warning, TEXT("Ship %s Patrol fallback bind on ShipRadius (%s)"),
            *GetActorNameOrLabel(),
            *ShipRadius->GetName());
#endif
    }

    GetWorldTimerManager().SetTimer(WithinScanerUpdateTimer, this, &AShip::UpdateWithinScaner, 0.25f, true);
    UpdateWithinScaner();

    // NAVIGATION_TODO_REMOVE
    ShipController = Cast<AAIShipController>(GetController());
    if (!ShipController)
    {
        UE_LOG(LogTemp, Warning, TEXT("ShipController not found on %s"), *GetName());
        bLoggedMissingController = true;
    }

    if (bApplyPresetOnBeginPlay && ShipPreset != EShipPreset::Custom)
    {
        ApplyShipPreset();
    }
    else
    {
        SyncControllerTuningFromShip();
        SyncManualTuningFromShip();
    }
#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Log, TEXT("Ship %s DebugSteering=%s DebugOrbit=%s"),
        *GetActorNameOrLabel(),
        bDebugSteering ? TEXT("true") : TEXT("false"),
        bDebugOrbit ? TEXT("true") : TEXT("false"));
#endif
}

#if WITH_EDITOR
void AShip::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.Property
        ? PropertyChangedEvent.Property->GetFName()
        : NAME_None;

    if (PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ShipPreset))
    {
        if (ShipPreset != EShipPreset::Custom)
        {
            ApplyShipPreset();
            ShipPreset = EShipPreset::Custom;
        }
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(AShip, MaxPitchSpeed)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, MaxYawSpeed)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, PitchAccelSpeed)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, YawAccelSpeed)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, MaxRollSpeed)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, RollAccelSpeed)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, MaxRollAngle)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, bUseTorquePD)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ControllerTorqueKpPitch)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ControllerTorqueKpYaw)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ControllerTorqueKdPitch)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ControllerTorqueKdYaw)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ControllerMaxTorquePitch)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ControllerMaxTorqueYaw)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ControllerMaxTorqueRoll)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ControllerTorqueRollDamping)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, RollAlignKp)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, RollAlignKd)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, MaxForwardForce)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, MinThrottle)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, ThrottleInterpSpeed)
        || PropertyName == GET_MEMBER_NAME_CHECKED(AShip, LateralDamping))
    {
        SyncControllerTuningFromShip();
        SyncManualTuningFromShip();
    }
}
#endif

FShipTuningSnapshot AShip::BuildTuningSnapshot() const
{
    FShipTuningSnapshot Snapshot;
    Snapshot.MaxPitchSpeed = MaxPitchSpeed;
    Snapshot.MaxYawSpeed = MaxYawSpeed;
    Snapshot.PitchAccelSpeed = PitchAccelSpeed;
    Snapshot.YawAccelSpeed = YawAccelSpeed;
    Snapshot.MaxRollSpeed = MaxRollSpeed;
    Snapshot.RollAccelSpeed = RollAccelSpeed;
    Snapshot.MaxRollAngle = MaxRollAngle;
    Snapshot.bUseTorquePD = bUseTorquePD;
    Snapshot.TorqueKpPitch = ControllerTorqueKpPitch;
    Snapshot.TorqueKpYaw = ControllerTorqueKpYaw;
    Snapshot.TorqueKdPitch = ControllerTorqueKdPitch;
    Snapshot.TorqueKdYaw = ControllerTorqueKdYaw;
    Snapshot.MaxTorquePitch = ControllerMaxTorquePitch;
    Snapshot.MaxTorqueYaw = ControllerMaxTorqueYaw;
    Snapshot.MaxTorqueRoll = ControllerMaxTorqueRoll;
    Snapshot.TorqueRollDamping = ControllerTorqueRollDamping;
    Snapshot.RollAlignKp = RollAlignKp;
    Snapshot.RollAlignKd = RollAlignKd;
    Snapshot.MaxForwardForce = MaxForwardForce;
    Snapshot.MinThrottle = MinThrottle;
    Snapshot.ThrottleInterpSpeed = ThrottleInterpSpeed;
    Snapshot.LateralDamping = LateralDamping;
    return Snapshot;
}

void AShip::ApplyTuningSnapshot(const FShipTuningSnapshot& Snapshot)
{
    MaxPitchSpeed = Snapshot.MaxPitchSpeed;
    MaxYawSpeed = Snapshot.MaxYawSpeed;
    PitchAccelSpeed = Snapshot.PitchAccelSpeed;
    YawAccelSpeed = Snapshot.YawAccelSpeed;
    MaxRollSpeed = Snapshot.MaxRollSpeed;
    RollAccelSpeed = Snapshot.RollAccelSpeed;
    MaxRollAngle = Snapshot.MaxRollAngle;
    bUseTorquePD = Snapshot.bUseTorquePD;
    ControllerTorqueKpPitch = Snapshot.TorqueKpPitch;
    ControllerTorqueKpYaw = Snapshot.TorqueKpYaw;
    ControllerTorqueKdPitch = Snapshot.TorqueKdPitch;
    ControllerTorqueKdYaw = Snapshot.TorqueKdYaw;
    ControllerMaxTorquePitch = Snapshot.MaxTorquePitch;
    ControllerMaxTorqueYaw = Snapshot.MaxTorqueYaw;
    ControllerMaxTorqueRoll = Snapshot.MaxTorqueRoll;
    ControllerTorqueRollDamping = Snapshot.TorqueRollDamping;
    RollAlignKp = Snapshot.RollAlignKp;
    RollAlignKd = Snapshot.RollAlignKd;
    MaxForwardForce = Snapshot.MaxForwardForce;
    MinThrottle = Snapshot.MinThrottle;
    ThrottleInterpSpeed = Snapshot.ThrottleInterpSpeed;
    LateralDamping = Snapshot.LateralDamping;
    TuningSnapshot = Snapshot;
}

void AShip::SyncControllerTuningFromShip()
{
    TuningSnapshot = BuildTuningSnapshot();
    ApplyControllerTuning();
}

void AShip::SyncManualTuningFromShip()
{
    bManualUseTorquePD = bUseTorquePD;
    ManualTorqueKpPitch = ControllerTorqueKpPitch;
    ManualTorqueKpYaw = ControllerTorqueKpYaw;
    ManualTorqueKdPitch = ControllerTorqueKdPitch;
    ManualTorqueKdYaw = ControllerTorqueKdYaw;
    ManualMaxTorquePitch = ControllerMaxTorquePitch;
    ManualMaxTorqueYaw = ControllerMaxTorqueYaw;
    ManualTorqueRollDamping = ControllerTorqueRollDamping;
    ManualMaxTorqueRoll = ControllerMaxTorqueRoll;

    if (EnsureShipController())
    {
        ManualAimDeadZoneDeg = ShipController->GetAimDeadZoneDeg();
    }
}

void AShip::ApplyControllerTuning()
{
    if (!EnsureShipController())
    {
        return;
    }

    ShipController->SetTorqueTuning(
        bUseTorquePD,
        ControllerTorqueKpPitch,
        ControllerTorqueKpYaw,
        ControllerTorqueKdPitch,
        ControllerTorqueKdYaw,
        ControllerMaxTorquePitch,
        ControllerMaxTorqueYaw,
        ControllerMaxTorqueRoll,
        ControllerTorqueRollDamping);
}

void AShip::ApplyShipPreset()
{
    if (ShipPreset == EShipPreset::Custom)
    {
        return;
    }

    float TorqueKpPitch = 18.f;
    float TorqueKpYaw = 22.f;
    float TorqueKdPitch = 6.f;
    float TorqueKdYaw = 7.f;
    float MaxTorquePitch = 1.f;
    float MaxTorqueYaw = 1.f;
    float MaxTorqueRoll = 1.f;
    float TorqueRollDamping = 0.2f;

    switch (ShipPreset)
    {
        case EShipPreset::Fighter:
        {
            MaxForwardForce = 2850.f;
            MinThrottle = 0.15f;
            ThrottleInterpSpeed = 10.f;
            LateralDamping = 0.5f;
            MaxPitchSpeed = 18.f;
            MaxYawSpeed = 18.f;
            PitchAccelSpeed = 8.f;
            YawAccelSpeed = 15.f;
            MaxRollSpeed = 20.f;
            RollAccelSpeed = 15.f;
            MaxRollAngle = 22.5f;
            break;
        }
        case EShipPreset::Interceptor:
        {
            MaxForwardForce = 3150.f;
            MinThrottle = 0.17f;
            ThrottleInterpSpeed = 9.f;
            LateralDamping = 0.6f;
            MaxPitchSpeed = 17.f;
            MaxYawSpeed = 17.f;
            PitchAccelSpeed = 7.5f;
            YawAccelSpeed = 13.5f;
            MaxRollSpeed = 18.f;
            RollAccelSpeed = 13.5f;
            MaxRollAngle = 20.f;
            TorqueKpPitch = 15.f;
            TorqueKpYaw = 19.f;
            TorqueKdPitch = 5.f;
            TorqueKdYaw = 6.f;
            MaxTorquePitch = 0.85f;
            MaxTorqueYaw = 0.85f;
            MaxTorqueRoll = 0.85f;
            TorqueRollDamping = 0.23f;
            break;
        }
        case EShipPreset::Gunship:
        {
            MaxForwardForce = 2100.f;
            MinThrottle = 0.22f;
            ThrottleInterpSpeed = 6.f;
            LateralDamping = 1.0f;
            MaxPitchSpeed = 8.5f;
            MaxYawSpeed = 8.5f;
            PitchAccelSpeed = 3.5f;
            YawAccelSpeed = 6.f;
            MaxRollSpeed = 8.5f;
            RollAccelSpeed = 6.f;
            MaxRollAngle = 12.f;
            TorqueKpPitch = 9.f;
            TorqueKpYaw = 10.5f;
            TorqueKdPitch = 3.8f;
            TorqueKdYaw = 4.2f;
            MaxTorquePitch = 0.55f;
            MaxTorqueYaw = 0.55f;
            MaxTorqueRoll = 0.55f;
            TorqueRollDamping = 0.3f;
            break;
        }
        case EShipPreset::Cruiser:
        {
            MaxForwardForce = 1560.f;
            MinThrottle = 0.24f;
            ThrottleInterpSpeed = 4.5f;
            LateralDamping = 1.3f;
            MaxPitchSpeed = 6.f;
            MaxYawSpeed = 6.f;
            PitchAccelSpeed = 2.8f;
            YawAccelSpeed = 4.5f;
            MaxRollSpeed = 6.f;
            RollAccelSpeed = 4.5f;
            MaxRollAngle = 9.5f;
            TorqueKpPitch = 6.5f;
            TorqueKpYaw = 7.5f;
            TorqueKdPitch = 3.f;
            TorqueKdYaw = 3.4f;
            MaxTorquePitch = 0.4f;
            MaxTorqueYaw = 0.4f;
            MaxTorqueRoll = 0.4f;
            TorqueRollDamping = 0.35f;
            break;
        }
        case EShipPreset::Carrier:
        {
            MaxForwardForce = 1290.f;
            MinThrottle = 0.26f;
            ThrottleInterpSpeed = 3.5f;
            LateralDamping = 1.6f;
            MaxPitchSpeed = 4.f;
            MaxYawSpeed = 4.f;
            PitchAccelSpeed = 2.f;
            YawAccelSpeed = 3.2f;
            MaxRollSpeed = 4.f;
            RollAccelSpeed = 3.f;
            MaxRollAngle = 7.f;
            TorqueKpPitch = 4.5f;
            TorqueKpYaw = 5.5f;
            TorqueKdPitch = 2.4f;
            TorqueKdYaw = 2.8f;
            MaxTorquePitch = 0.28f;
            MaxTorqueYaw = 0.28f;
            MaxTorqueRoll = 0.28f;
            TorqueRollDamping = 0.4f;
            break;
        }
        default:
            break;
    }

    bUseTorquePD = true;
    ControllerTorqueKpPitch = TorqueKpPitch;
    ControllerTorqueKpYaw = TorqueKpYaw;
    ControllerTorqueKdPitch = TorqueKdPitch;
    ControllerTorqueKdYaw = TorqueKdYaw;
    ControllerMaxTorquePitch = MaxTorquePitch;
    ControllerMaxTorqueYaw = MaxTorqueYaw;
    ControllerMaxTorqueRoll = MaxTorqueRoll;
    ControllerTorqueRollDamping = TorqueRollDamping;
    SyncControllerTuningFromShip();
    SyncManualTuningFromShip();

    if (Vitality)
    {
        Vitality->ApplyVitalityPreset(ShipPreset);
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

    if (bManualControl)
    {
        ApplyManualControl(DeltaTime);
    }
    else
    {
        const bool bMovementAllowed = ShipController && ShipController->IsMovementAllowed();

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
        const float DistanceToGoal = FVector::Dist(ActorLocation, Goal);
        const bool bInSafetyMargin = ShipController && ShipController->IsInsideSafetyMargin();
        const bool bIsUnstuck = ShipController && ShipController->IsUnstucking();
        const bool bSafetyMarginAllowed = SafetyForceNavCooldown <= 0.0f;
        const bool bSafetyMarginActive = bInSafetyMargin && bSafetyMarginAllowed;
        if (SafetyForceNavCooldown > 0.0f)
        {
            SafetyForceNavCooldown = FMath::Max(0.0f, SafetyForceNavCooldown - DeltaTime);
        }
        if (bSafetyMarginActive || bIsUnstuck)
        {
            if (SafetyStateTimeAccumulated <= 0.0f)
            {
                SafetyStatePrevGoalDistance = DistanceToGoal;
            }
            if (DistanceToGoal < (SafetyStatePrevGoalDistance - 10.0f))
            {
                SafetyStatePrevGoalDistance = DistanceToGoal;
                SafetyStateTimeAccumulated = 0.0f;
            }
            else
            {
                SafetyStateTimeAccumulated += DeltaTime;
            }
        }
        else
        {
            SafetyStatePrevGoalDistance = DistanceToGoal;
            SafetyStateTimeAccumulated = 0.0f;
        }
        bool bForceNav = false;
        if ((bSafetyMarginActive || bIsUnstuck) && SafetyStateTimeAccumulated >= 1.0f)
        {
            bForceNav = true;
            SafetyStateTimeAccumulated = 0.0f;
            SafetyForceNavCooldown = 1.0f;
            if (ShipController)
            {
                ShipController->SetCurrentObstacleComp(nullptr);
                ShipController->SuppressSafetyMargin(ShipController->GetSafetyMarginSuppressDuration());
            }
#if !UE_BUILD_SHIPPING
            UE_LOG(LogTemp, Warning, TEXT("Ship %s forcing Nav fallback (state=%s Dist=%.1f)"),
                *GetActorNameOrLabel(),
                bInSafetyMargin ? TEXT("SafetyMargin") : TEXT("Unstuck"),
                DistanceToGoal);
#endif
        }
#if !UE_BUILD_SHIPPING
        if (bDebugSteering)
        {
            SafetyStateLogAccumulator += DeltaTime;
            if (SafetyStateLogAccumulator >= 1.0f)
            {
                SafetyStateLogAccumulator = 0.0f;
                const TCHAR* StateLabel = bSafetyMarginActive ? TEXT("SafetyMargin") : (bIsUnstuck ? TEXT("Unstuck") : TEXT("Nav"));
                UE_LOG(LogTemp, Verbose, TEXT("Ship %s NavState=%s Dist=%.1f"),
                    *GetActorNameOrLabel(),
                    StateLabel,
                    DistanceToGoal);
            }
        }
#endif
#if !UE_BUILD_SHIPPING
/*         if ((Goal - ActorLocation).IsNearlyZero(1.f))
        {
            DebugMessageAccumulator += DeltaTime;
            if (DebugMessageAccumulator >= 1.f)
            {
                DebugMessageAccumulator = 0.f;
                UE_LOG(LogTemp, Warning, TEXT("Ship %s has goal equal to self."), *GetActorNameOrLabel());
            }
        } */
#endif
		const float ShipRadiusCm = ShipRadius ? ShipRadius->GetScaledSphereRadius() : 300.f;
		FVector SteeringTarget = Goal;
		const TCHAR* SteeringSource = TEXT("Goal");
		if (!bForceNav && bSafetyMarginActive)
        {
            SteeringTarget = ShipController->ComputeEscapeTarget(ActorLocation, ShipRadius);
            SteeringSource = TEXT("SafetyMargin");
#if !UE_BUILD_SHIPPING
            const bool bTargetIsSelf = SteeringTarget.Equals(ActorLocation, 1.f);
            const bool bTargetIsZero = SteeringTarget.IsNearlyZero();
            const bool bTargetIsFinite = SteeringTarget.ContainsNaN() == false;
            if (bTargetIsSelf || bTargetIsZero || !bTargetIsFinite)
            {
                UE_LOG(LogTemp, Warning, TEXT("Ship %s invalid EscapeTarget (Self=%s Zero=%s Finite=%s) - forcing Nav"),
                    *GetActorNameOrLabel(),
                    bTargetIsSelf ? TEXT("true") : TEXT("false"),
                    bTargetIsZero ? TEXT("true") : TEXT("false"),
                    bTargetIsFinite ? TEXT("true") : TEXT("false"));
                bForceNav = true;
                SteeringSource = TEXT("Nav");
                if (ShipController)
                {
                    ShipController->SetCurrentObstacleComp(nullptr);
                    ShipController->SuppressSafetyMargin(ShipController->GetSafetyMarginSuppressDuration());
                }
            }
#endif
		}
		else if (!bForceNav && bIsUnstuck)
		{
			SteeringTarget = ShipController->ComputeEscapeTarget(ActorLocation, ShipRadius);
            SteeringSource = TEXT("Unstuck");
        }
        else if (ShipNav && (!ShipController || !ShipController->IsUnstucking()))
        {
            ShipNav->TickNav(DeltaTime, Goal, ShipRadiusCm, bMovingGoal);
            SteeringTarget = ShipNav->GetNavTarget(Goal);
            SteeringSource = TEXT("Nav");
        }

#if !UE_BUILD_SHIPPING
        if (bDebugSteering)
        {
            DebugSteeringAccumulator += DeltaTime;
            if (DebugSteeringAccumulator >= 0.5f)
            {
                DebugSteeringAccumulator = 0.f;
                const FVector Forward = ShipBase ? ShipBase->GetForwardVector() : FVector::ForwardVector;
                const FVector ToTarget = (SteeringTarget - ActorLocation).GetSafeNormal();
                const float Heading = FVector::DotProduct(Forward, ToTarget);
                const float Distance = FVector::Dist(ActorLocation, SteeringTarget);
                const float Speed = ShipBase ? ShipBase->GetPhysicsLinearVelocity().Size() : 0.0f;
                UE_LOG(LogTemp, Log,
                    TEXT("Ship %s Steer[%s] Dist=%.1f Speed=%.1f Heading=%.2f Target=%s"),
                    *GetActorNameOrLabel(),
                    SteeringSource,
                    Distance,
                    Speed,
                    Heading,
                    *SteeringTarget.ToString());
            }
        }
#endif
        if (bMovementAllowed)
        {
            ApplySteeringForce(SteeringTarget, DeltaTime);
        }
        else
        {
            CurrentThrottle = 0.0f;
        }

#if !UE_BUILD_SHIPPING
        if (bDebugOrbit && GetWorld())
        {
            DrawDebugPoint(GetWorld(), Goal, 14.f, FColor::Yellow, false, 0.f, 0);
            DrawDebugPoint(GetWorld(), SteeringTarget, 14.f, FColor::Cyan, false, 0.f, 0);
        }
#endif

        // Rotation
        // NAVIGATION_TODO_REMOVE
        if (bMovementAllowed && ShipController)
        {
            ShipController->ApplyShipRotation(SteeringTarget);
        }
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

void AShip::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);

    if (NewController && NewController->IsPlayerController())
    {
        StoredAIController = ShipController;
        SyncManualTuningFromShip();
        bManualControl = true;
    }

    ShipController = Cast<AAIShipController>(GetController());
    SyncControllerTuningFromShip();
}

void AShip::UnPossessed()
{
    Super::UnPossessed();

    if (bManualControl)
    {
        bManualControl = false;
        ManualPitchInput = 0.f;
        ManualYawInput = 0.f;
        ManualRollInput = 0.f;
        ManualThrottleInput = 0.f;
        bManualUseTorquePD = false;
    }

    if (StoredAIController.IsValid() && GetController() != StoredAIController.Get())
    {
        StoredAIController->Possess(this);
    }
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

	if (OtherActor == this || OtherComp == ShipBase || OtherComp == ShipRadius)
	{
		return;
	}

    // Update overlap filtering logic to also accept Pawns as blockers
    const bool bBlocksAvoidance = OtherComp->GetCollisionResponseToChannel(ECC_GameTraceChannel3) == ECR_Block;
    const bool bIsPawn = OtherActor && OtherActor->IsA<APawn>();
    
    if (!bBlocksAvoidance && !bIsPawn)
    {
        return;
    }

	// Add to soft separation bookkeeping (avoid duplicates)
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

void AShip::HandlePatrolPointBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Warning, TEXT("Ship %s PatrolOverlap: OtherActor=%s OtherComp=%s"),
        *GetActorNameOrLabel(),
        OtherActor ? *OtherActor->GetActorNameOrLabel() : TEXT("None"),
        *OtherComp->GetName());
#endif
    if (!OtherComp || OtherActor == this)
    {
        return;
    }



    if (EnsureShipController())
    {
        ShipController->HandlePatrolPointOverlap(OtherComp, OtherActor);
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
        if (ShipController->IsPatrolInProgress())
        {
            if (ANavStaticBig* PatrolPoint = ShipController->GetCurrentPatrolPoint())
            {
                return PatrolPoint->GetActorLocation();
            }

            return GetActorLocation();
        }

        return ShipController->GetFocusLocation();
    }

    return GetActorLocation();
}

// NAVIGATION_TODO_REMOVE
void AShip::ApplySteeringForce(FVector TargetLocation, float DeltaTime, bool bOverrideThrottle, float ThrottleOverride)
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
/*         if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(
                -1,
                1.f,
                FColor::Yellow,
                TEXT("No valid target (goal == self)")
            );
        } */
#endif
        return;
    }

    const FVector Forward = ShipBase->GetForwardVector();
    const float Heading = FMath::Clamp(FVector::DotProduct(Forward, ToTarget), -1.f, 1.f);

    const float ThrottleTarget = bOverrideThrottle
        ? FMath::Clamp(ThrottleOverride, -1.f, 1.f)
        : FMath::Clamp(Heading, MinThrottle, 1.f);
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

void AShip::SetManualRotationInput(float Pitch, float Yaw, float Roll)
{
    ManualPitchInput = FMath::Clamp(Pitch, -1.f, 1.f);
    ManualYawInput = FMath::Clamp(Yaw, -1.f, 1.f);
    ManualRollInput = FMath::Clamp(Roll, -1.f, 1.f);
}

void AShip::SetManualThrottleInput(float Value)
{
    ManualThrottleInput = FMath::Clamp(Value, -1.f, 1.f);
}

void AShip::ApplyRotationServo(const FVector& TargetAngVelLocal, float DeltaTime)
{
    if (!ShipBase || DeltaTime <= 0.f)
    {
        return;
    }

    const float PitchResponse = FMath::Max(PitchAccelSpeed, 0.1f);
    const float YawResponse = FMath::Max(YawAccelSpeed, 0.1f);
    const float RollResponse = FMath::Max(RollAccelSpeed, 0.1f);

    const FTransform& ShipBaseTransform = ShipBase->GetComponentTransform();
    const FVector CurAngVelWorld = ShipBase->GetPhysicsAngularVelocityInDegrees();
    const FVector CurAngVelLocal = ShipBaseTransform.InverseTransformVectorNoScale(CurAngVelWorld);

    FVector NewAngVelLocal = CurAngVelLocal;
    NewAngVelLocal.X = FMath::FInterpTo(CurAngVelLocal.X, TargetAngVelLocal.X, DeltaTime, RollResponse);
    NewAngVelLocal.Y = FMath::FInterpTo(CurAngVelLocal.Y, TargetAngVelLocal.Y, DeltaTime, PitchResponse);
    NewAngVelLocal.Z = FMath::FInterpTo(CurAngVelLocal.Z, TargetAngVelLocal.Z, DeltaTime, YawResponse);

    const FVector NewAngVelWorld = ShipBaseTransform.TransformVectorNoScale(NewAngVelLocal);
    ShipBase->SetPhysicsAngularVelocityInDegrees(NewAngVelWorld, false);
}

void AShip::ApplyManualControl(float DeltaTime)
{
    if (!ShipBase || DeltaTime <= 0.f)
    {
        return;
    }

    const FVector Forward = ShipBase->GetForwardVector();
    const FVector Right = ShipBase->GetRightVector();
    const FVector Up = ShipBase->GetUpVector();

    FVector InputDirection =
        Forward
        + (Right * ManualYawInput)
        + (Up * ManualPitchInput);

    if (InputDirection.IsNearlyZero())
    {
        InputDirection = Forward;
    }

    const FVector TargetLocation = GetActorLocation() + InputDirection * 10000.f;
    ApplySteeringForce(TargetLocation, DeltaTime, true, ManualThrottleInput);
    ApplyManualShipRotation(TargetLocation, DeltaTime);
}

void AShip::ApplyManualShipRotation(const FVector& TargetLocation, float DeltaTime)
{
    if (!ShipBase || DeltaTime <= 0.f)
    {
        return;
    }

    const FVector ActorLocation = GetActorLocation();
    FVector ToTargetWorld = (TargetLocation - ActorLocation);
    if (ToTargetWorld.IsNearlyZero())
    {
        return;
    }
    ToTargetWorld.Normalize();

    const FTransform& ShipBaseTransform = ShipBase->GetComponentTransform();
    const FVector ToTargetLocal = ShipBaseTransform.InverseTransformVectorNoScale(ToTargetWorld);
    if (ToTargetLocal.IsNearlyZero())
    {
        return;
    }

    const FVector ToTargetLocalDir = ToTargetLocal.GetSafeNormal();

    float RollErrRad = 0.f;
    if (bManualUseRollAlign && RollAlignMode != ERollAlignMode::Default)
    {
        FVector DesiredUpLocal = FVector::UpVector;

        if (RollAlignMode == ERollAlignMode::BackToTarget)
            DesiredUpLocal = ToTargetLocalDir;
        else if (RollAlignMode == ERollAlignMode::BellyToTarget)
            DesiredUpLocal = -ToTargetLocalDir;

        DesiredUpLocal.X = 0.f;
        if (DesiredUpLocal.IsNearlyZero())
            DesiredUpLocal = FVector(0.f, 0.f, 1.f);
        DesiredUpLocal.Normalize();

        const FVector CurrentUpLocal(0.f, 0.f, 1.f);

        const float CrossX = FVector::CrossProduct(CurrentUpLocal, DesiredUpLocal).X;
        const float DotUD  = FVector::DotProduct(CurrentUpLocal, DesiredUpLocal);
        RollErrRad = FMath::Atan2(CrossX, DotUD);
    }

    const bool bTargetBehind = (ToTargetLocalDir.X < 0.f);

    float PitchErr = -FMath::RadiansToDegrees(
        FMath::Atan2(ToTargetLocalDir.Z, ToTargetLocalDir.X)
    );
    float YawErr = FMath::RadiansToDegrees(
        FMath::Atan2(ToTargetLocalDir.Y, ToTargetLocalDir.X)
    );

    if (bTargetBehind)
    {
        float PreferredYawSign = 0.f;
        const FVector CurAngVelWorldDeg = ShipBase->GetPhysicsAngularVelocityInDegrees();
        const FVector CurAngVelLocalDeg = ShipBaseTransform.InverseTransformVectorNoScale(CurAngVelWorldDeg);
        if (FMath::Abs(CurAngVelLocalDeg.Z) > 1.f)
        {
            PreferredYawSign = FMath::Sign(CurAngVelLocalDeg.Z);
        }
        if (FMath::IsNearlyZero(PreferredYawSign))
        {
            PreferredYawSign = (YawErr >= 0.f) ? 1.f : -1.f;
        }

        YawErr = FMath::Abs(YawErr) * PreferredYawSign;
        PitchErr *= 0.5f;
    }

    float PitchErrDz = (FMath::Abs(PitchErr) < ManualAimDeadZoneDeg) ? 0.f : PitchErr;
    float YawErrDz   = (FMath::Abs(YawErr)   < ManualAimDeadZoneDeg) ? 0.f : YawErr;

    if (bManualUseTorquePD)
    {
        const FVector CurAngVelWorldDeg = ShipBase->GetPhysicsAngularVelocityInDegrees();
        const FVector CurAngVelLocalDeg = ShipBaseTransform.InverseTransformVectorNoScale(CurAngVelWorldDeg);
        const FVector CurAngVelLocalRad = CurAngVelLocalDeg * (PI / 180.f);

        const float PitchErrRad = FMath::DegreesToRadians(PitchErrDz);
        const float YawErrRad   = FMath::DegreesToRadians(YawErrDz);

        const bool bAlmostAimed =
            (FMath::Abs(PitchErrDz) < (ManualAimDeadZoneDeg * 2.f)) &&
            (FMath::Abs(YawErrDz)   < (ManualAimDeadZoneDeg * 2.f));

        const FVector Inertia = ShipBase->GetInertiaTensor();
        const float Ix = FMath::Max(Inertia.X, 1.f);
        const float Iy = FMath::Max(Inertia.Y, 1.f);
        const float Iz = FMath::Max(Inertia.Z, 1.f);

        float AlphaPitch = 0.f;
        float AlphaYaw = 0.f;

        if (bAlmostAimed)
        {
            AlphaPitch = -(ManualTorqueKdPitch * 1.5f) * CurAngVelLocalRad.Y;
            AlphaYaw   = -(ManualTorqueKdYaw   * 1.5f) * CurAngVelLocalRad.Z;
        }
        else
        {
            AlphaPitch = (ManualTorqueKpPitch * PitchErrRad) - (ManualTorqueKdPitch * CurAngVelLocalRad.Y);
            AlphaYaw   = (ManualTorqueKpYaw   * YawErrRad)   - (ManualTorqueKdYaw   * CurAngVelLocalRad.Z);
        }

        float TorquePitch = Iy * AlphaPitch;
        float TorqueYaw = Iz * AlphaYaw;

        float TorqueRoll = 0.f;
        if (FMath::Abs(ManualRollInput) > KINDA_SMALL_NUMBER)
        {
            const float RollTargetRad = ManualRollInput * MaxRollSpeed * (PI / 180.f);
            const float RollErrRadManual = RollTargetRad - CurAngVelLocalRad.X;
            TorqueRoll = (Ix * (ManualTorqueKpPitch * RollErrRadManual))
                - (Ix * (ManualTorqueRollDamping * CurAngVelLocalRad.X));
        }
        else if (!bManualUseRollAlign)
        {
            const float RollTargetRad = -ManualYawInput * ManualYawBankScale * MaxRollSpeed * (PI / 180.f);
            const float RollErrRadManual = RollTargetRad - CurAngVelLocalRad.X;
            TorqueRoll = (Ix * (ManualTorqueKpPitch * RollErrRadManual))
                - (Ix * (ManualTorqueRollDamping * CurAngVelLocalRad.X));
        }
        else if (bManualUseRollAlign && RollAlignMode != ERollAlignMode::Default)
        {
            const float AlphaRoll =
                (RollAlignKp * RollErrRad) - (RollAlignKd * CurAngVelLocalRad.X);
            TorqueRoll = Ix * AlphaRoll;
        }
        else
        {
            TorqueRoll = -Ix * (ManualTorqueRollDamping * CurAngVelLocalRad.X);
        }

        TorquePitch = FMath::Clamp(TorquePitch, -ManualMaxTorquePitch, ManualMaxTorquePitch);
        TorqueYaw = FMath::Clamp(TorqueYaw, -ManualMaxTorqueYaw, ManualMaxTorqueYaw);
        TorqueRoll = FMath::Clamp(TorqueRoll, -ManualMaxTorqueRoll, ManualMaxTorqueRoll);

        const FVector TorqueLocal(TorqueRoll, TorquePitch, TorqueYaw);
        const FVector TorqueWorld = ShipBaseTransform.TransformVectorNoScale(TorqueLocal);
        ShipBase->AddTorqueInRadians(TorqueWorld, NAME_None, true);
        return;
    }

    const float DesiredPitchRate = FMath::Clamp(PitchErrDz * 0.5f, -MaxPitchSpeed, MaxPitchSpeed);
    const float DesiredYawRate   = FMath::Clamp(YawErrDz   * 0.55f, -MaxYawSpeed,  MaxYawSpeed);

    float DesiredRollRate = 0.f;
    if (FMath::Abs(ManualRollInput) > KINDA_SMALL_NUMBER)
    {
        DesiredRollRate = ManualRollInput * MaxRollSpeed;
    }
    else if (!bManualUseRollAlign)
    {
        DesiredRollRate = -ManualYawInput * ManualYawBankScale * MaxRollSpeed;
    }
    else if (bManualUseRollAlign && RollAlignMode != ERollAlignMode::Default)
    {
        const float RollErrDeg = FMath::RadiansToDegrees(RollErrRad);
        DesiredRollRate = FMath::Clamp(RollErrDeg * 0.5f, -MaxRollSpeed, MaxRollSpeed);
    }

    const FVector TargetAngVelLocal(DesiredRollRate, DesiredPitchRate, DesiredYawRate);
    ApplyRotationServo(TargetAngVelLocal, DeltaTime);
}

void AShip::ApplyManualTorqueControl(const FVector& TargetAngVelLocal, float DeltaTime)
{
    if (!ShipBase || DeltaTime <= 0.f)
    {
        return;
    }

    const FTransform& ShipBaseTransform = ShipBase->GetComponentTransform();
    const FVector CurAngVelWorldDeg = ShipBase->GetPhysicsAngularVelocityInDegrees();
    const FVector CurAngVelLocalDeg = ShipBaseTransform.InverseTransformVectorNoScale(CurAngVelWorldDeg);
    const FVector CurAngVelLocalRad = CurAngVelLocalDeg * (PI / 180.f);

    const FVector TargetAngVelLocalRad = TargetAngVelLocal * (PI / 180.f);
    const FVector ErrorLocalRad = TargetAngVelLocalRad - CurAngVelLocalRad;

    const FVector Inertia = ShipBase->GetInertiaTensor();
    const float Ix = FMath::Max(Inertia.X, 1.f);
    const float Iy = FMath::Max(Inertia.Y, 1.f);
    const float Iz = FMath::Max(Inertia.Z, 1.f);

    float AlphaPitch = (ManualTorqueKpPitch * ErrorLocalRad.Y) - (ManualTorqueKdPitch * CurAngVelLocalRad.Y);
    float AlphaYaw = (ManualTorqueKpYaw * ErrorLocalRad.Z) - (ManualTorqueKdYaw * CurAngVelLocalRad.Z);

    float TorquePitch = Iy * AlphaPitch;
    float TorqueYaw = Iz * AlphaYaw;

    float TorqueRoll = 0.f;
    if (RollAlignMode == ERollAlignMode::Default)
    {
        const float RollRateError = ErrorLocalRad.X;
        TorqueRoll = (Ix * (ManualTorqueKpPitch * RollRateError)) - (Ix * (ManualTorqueRollDamping * CurAngVelLocalRad.X));
    }

    TorquePitch = FMath::Clamp(TorquePitch, -ManualMaxTorquePitch, ManualMaxTorquePitch);
    TorqueYaw = FMath::Clamp(TorqueYaw, -ManualMaxTorqueYaw, ManualMaxTorqueYaw);
    TorqueRoll = FMath::Clamp(TorqueRoll, -ManualMaxTorqueRoll, ManualMaxTorqueRoll);

    const FVector TorqueLocal(TorqueRoll, TorquePitch, TorqueYaw);
    const FVector TorqueWorld = ShipBaseTransform.TransformVectorNoScale(TorqueLocal);
    ShipBase->AddTorqueInRadians(TorqueWorld, NAME_None, true);
}

void AShip::CacheManualRotationTuning(const AAIShipController* PreviousController)
{
    if (!PreviousController)
    {
        bManualUseTorquePD = false;
        return;
    }

    bManualUseTorquePD = PreviousController->UsesTorquePD();
    ManualTorqueKpPitch = PreviousController->GetTorqueKpPitch();
    ManualTorqueKpYaw = PreviousController->GetTorqueKpYaw();
    ManualTorqueKdPitch = PreviousController->GetTorqueKdPitch();
    ManualTorqueKdYaw = PreviousController->GetTorqueKdYaw();
    ManualMaxTorquePitch = PreviousController->GetMaxTorquePitch();
    ManualMaxTorqueYaw = PreviousController->GetMaxTorqueYaw();
    ManualTorqueRollDamping = PreviousController->GetTorqueRollDamping();
    ManualMaxTorqueRoll = PreviousController->GetMaxTorqueRoll();
    ManualAimDeadZoneDeg = PreviousController->GetAimDeadZoneDeg();
}

bool AShip::EnsureShipController()
{
    if (!ShipController)
    {
        ShipController = Cast<AAIShipController>(GetController());
    }
    return ShipController != nullptr;
}

void AShip::UpdateWithinScaner()
{
    WithinScaner.Reset();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FVector ShipLocation = GetActorLocation();
    const float ScanRadius = InternalScanerRadius ? InternalScanerRadius->GetScaledSphereRadius() : 0.0f;
    const float ScanRadiusSq = ScanRadius * ScanRadius;

    TArray<AActor*> FoundActors;
    FoundActors.Reserve(64);

    UGameplayStatics::GetAllActorsOfClass(World, AShip::StaticClass(), FoundActors);

    TArray<AActor*> FoundNavStaticBig;
    UGameplayStatics::GetAllActorsOfClass(World, ANavStaticBig::StaticClass(), FoundNavStaticBig);
    FoundActors.Append(FoundNavStaticBig);

    for (AActor* Actor : FoundActors)
    {
        if (!IsValid(Actor))
        {
            continue;
        }

        const float DistanceSq = FVector::DistSquared(ShipLocation, Actor->GetActorLocation());
        if (DistanceSq <= ScanRadiusSq)
        {
            WithinScaner.Add(Actor);
        }
    }

    WithinScaner.Sort([ShipLocation](const AActor& A, const AActor& B)
    {
        return FVector::DistSquared(ShipLocation, A.GetActorLocation()) < FVector::DistSquared(ShipLocation, B.GetActorLocation());
    });
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
