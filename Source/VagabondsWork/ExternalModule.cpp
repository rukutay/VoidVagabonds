// Fill out your copyright notice in the Description page of Project Settings.

#include "ExternalModule.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

static float ClampAngleAroundCenterDeg(float CenterDeg, float DesiredDeg, float LimitDeg)
{
    // LimitDeg is total sweep (0..360). Clamp around CenterDeg by half-range.
    const float C = FRotator::NormalizeAxis(CenterDeg);
    const float D = FRotator::NormalizeAxis(DesiredDeg);

    if (LimitDeg >= 359.9f) return D;

    const float Half = FMath::Clamp(LimitDeg * 0.5f, 0.f, 179.9f);
    const float Delta = FMath::FindDeltaAngleDegrees(C, D);
    const float ClampedDelta = FMath::Clamp(Delta, -Half, +Half);
    return FRotator::NormalizeAxis(C + ClampedDelta);
}

static float ComputeInterceptTime(const FVector& RelPos, const FVector& TargetVel, float ProjectileSpeed)
{
    // Solve |RelPos + TargetVel * t| = ProjectileSpeed * t
    const float s = FMath::Max(ProjectileSpeed, 1.f);
    const float a = TargetVel.SizeSquared() - (s * s);
    const float b = 2.f * FVector::DotProduct(RelPos, TargetVel);
    const float c = RelPos.SizeSquared();

    // If a ~= 0 -> linear solution
    if (FMath::Abs(a) < 1e-6f)
    {
        // b * t + c = 0  (from a t^2 + b t + c = 0)
        if (FMath::Abs(b) < 1e-6f) return -1.f;
        const float t = -c / b;
        return (t > 0.f) ? t : -1.f;
    }

    const float Disc = (b*b) - 4.f*a*c;
    if (Disc < 0.f) return -1.f;

    const float SqrtDisc = FMath::Sqrt(Disc);
    const float t1 = (-b - SqrtDisc) / (2.f*a);
    const float t2 = (-b + SqrtDisc) / (2.f*a);

    // pick smallest positive time
    float t = -1.f;
    if (t1 > 0.f) t = t1;
    if (t2 > 0.f) t = (t < 0.f) ? t2 : FMath::Min(t, t2);
    return t;
}

static FVector ComputePredictedLoc(const FVector& Start, AActor* Target, float ProjectileSpeed)
{
    if (!Target) return FVector::ZeroVector;
    const FVector TargetLoc = Target->GetActorLocation();
    const FVector TargetVel = Target->GetVelocity();
    const FVector RelPos = TargetLoc - Start;

    const float t = ComputeInterceptTime(RelPos, TargetVel, ProjectileSpeed);
    if (t > 0.f)
    {
        return TargetLoc + TargetVel * t;
    }

    // fallback: straight-line travel time
    const float Dist = RelPos.Size();
    const float FallbackT = Dist / FMath::Max(ProjectileSpeed, 1.f);
    return TargetLoc + TargetVel * FallbackT;
}

// Sets default values
AExternalModule::AExternalModule()
{
	// Set this actor to not call Tick() every frame to improve performance
	PrimaryActorTick.bCanEverTick = false;

	// Create core component hierarchy
	ModuleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ModuleRoot"));
	SetRootComponent(ModuleRoot);

	PivotBase = CreateDefaultSubobject<USceneComponent>(TEXT("PivotBase"));
	PivotBase->SetupAttachment(ModuleRoot);

	PivotGun = CreateDefaultSubobject<USceneComponent>(TEXT("PivotGun"));
	PivotGun->SetupAttachment(PivotBase);

	MeshBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshBase"));
	MeshBase->SetupAttachment(PivotBase);

	MeshGun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshGun"));
	MeshGun->SetupAttachment(PivotGun);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(PivotGun);

	// Set default config values
	TargetActor = nullptr;
	bAutoAim = true;
	bManualAimStep = false;
	AimUpdateHz = 30.f;
	bLimitPitch = true;
	MaxPitchAbsDeg = 65.f;
	bLimitBaseYaw = false;
	BaseYawLimitDeg = 360.f;
	bLimitGunPitch = true;
	GunPitchLimitDeg = 130.f; // equals +-65 around center by default
	InitialBaseYawWorld = 0.f;
	InitialGunPitchRel = 0.f;
	bUseDegPerSecSpeed = false;
	YawDegPerSec = 180.f;
	PitchDegPerSec = 180.f;
	YawInterpSpeed = 8.f;
	PitchInterpSpeed = 8.f;
	bUseMuzzleAsStart = true;
	bDebugAim = false;
	DebugDrawDuration = 0.05f;
	DebugAccumTime = 0.0f;
}

// Called when the game starts or when spawned
void AExternalModule::BeginPlay()
{
	Super::BeginPlay();
	
	// Cache initial angles
	InitialBaseYawWorld = FRotator::NormalizeAxis(PivotBase ? PivotBase->GetComponentRotation().Yaw : 0.f);
	InitialGunPitchRel  = FRotator::NormalizeAxis(PivotGun ? PivotGun->GetRelativeRotation().Pitch : 0.f);
	
	// Only start timer if auto aim is enabled and manual aim step is disabled
	if (bAutoAim && !bManualAimStep && AimUpdateHz > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(AimTimerHandle, this, &AExternalModule::UpdateAim, 1.0f / AimUpdateHz, true);
	}
}

// Called when the actor is being removed from the game
void AExternalModule::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	GetWorld()->GetTimerManager().ClearTimer(AimTimerHandle);
}

void AExternalModule::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void AExternalModule::ClearTarget()
{
	TargetActor = nullptr;
}

void AExternalModule::AimStepManual(float DeltaSeconds)
{
	AimStep(DeltaSeconds);
}

FVector AExternalModule::GetMuzzleWorldLocation() const
{
	return Muzzle ? Muzzle->GetComponentLocation() : FVector::ZeroVector;
}

FVector AExternalModule::GetMuzzleWorldForward() const
{
	return Muzzle ? Muzzle->GetForwardVector() : FVector::ForwardVector;
}

float AExternalModule::GetCurrentYaw() const
{
	return PivotBase ? FRotator::NormalizeAxis(PivotBase->GetComponentRotation().Yaw) : 0.0f;
}

float AExternalModule::GetCurrentPitch() const
{
	return PivotGun ? FRotator::NormalizeAxis(PivotGun->GetRelativeRotation().Pitch) : 0.0f;
}

void AExternalModule::UpdateAim()
{
	if (!TargetActor) return;
	const float Dt = (AimUpdateHz > 0) ? (1.f / AimUpdateHz) : 0.f;
	AimStep(Dt);
}

void AExternalModule::AimStep(float Dt)
{
	if (!PivotBase || !PivotGun || !Muzzle || !TargetActor || Dt <= 0) return;

	const FVector Start = bUseMuzzleAsStart ? Muzzle->GetComponentLocation() : PivotGun->GetComponentLocation();

	FVector AimLoc = TargetActor->GetActorLocation();
	if (bUseLeadPrediction)
	{
		AimLoc = ComputePredictedLoc(Start, TargetActor, ProjectileInitialSpeed);
	}

	const FVector AimDirWorld = (AimLoc - Start).GetSafeNormal();
	
	// Get parent transform for PivotBase
	USceneComponent* BaseParent = PivotBase->GetAttachParent();
	if (!BaseParent) return;
	
	// Convert direction into BaseParent local space
	const FVector AimDirParent = BaseParent->GetComponentTransform().InverseTransformVectorNoScale(AimDirWorld);
	
	// Compute desired yaw (forward +X, right +Y)
	float DesiredYaw = FRotator::NormalizeAxis(FMath::RadiansToDegrees(FMath::Atan2(AimDirParent.Y, AimDirParent.X)));
	
	// Apply base yaw limit if enabled
	if (bLimitBaseYaw)
	{
		DesiredYaw = ClampAngleAroundCenterDeg(InitialBaseYawWorld, DesiredYaw, BaseYawLimitDeg);
	}
	
	// Apply yaw rotation based on speed mode
	if (!bUseDegPerSecSpeed)
	{
		// Use RInterpTo speeds (original behavior)
		const FRotator CurrentYawRel = PivotBase->GetRelativeRotation();
		const FRotator DesiredYawRel(0.f, DesiredYaw, 0.f);
		const FRotator NewYawRel = FMath::RInterpTo(CurrentYawRel, DesiredYawRel, Dt, YawInterpSpeed);
		PivotBase->SetRelativeRotation(FRotator(0.f, NewYawRel.Yaw, 0.f));
	}
	else
	{
		// Use deg/sec stepping with relative rotation
		const float CurrentYaw = FRotator::NormalizeAxis(PivotBase->GetRelativeRotation().Yaw);
		const float Delta = FMath::FindDeltaAngleDegrees(CurrentYaw, DesiredYaw);
		const float Step = FMath::Clamp(Delta, -YawDegPerSec * Dt, +YawDegPerSec * Dt);
		const float NewYaw = FRotator::NormalizeAxis(CurrentYaw + Step);
		PivotBase->SetRelativeRotation(FRotator(0.f, NewYaw, 0.f));
	}
	
	// Recompute pitch after yaw is applied
	// Convert AimDirWorld into PivotBase local space
	const FVector AimDirBase = PivotBase->GetComponentTransform().InverseTransformVectorNoScale(AimDirWorld);
	
	// Compute horizontal magnitude
	const float Horizontal = FMath::Sqrt(AimDirBase.X*AimDirBase.X + AimDirBase.Y*AimDirBase.Y);
	if (Horizontal < 1e-4f) return;
	
	// DesiredPitch (degrees)
	float DesiredPitch = FRotator::NormalizeAxis(FMath::RadiansToDegrees(FMath::Atan2(AimDirBase.Z, Horizontal)));
	
	// Apply gun pitch limit if enabled, otherwise use old pitch limit behavior
	if (bLimitGunPitch)
	{
		DesiredPitch = ClampAngleAroundCenterDeg(InitialGunPitchRel, DesiredPitch, GunPitchLimitDeg);
	}
	else if (bLimitPitch)
	{
		DesiredPitch = FMath::Clamp(DesiredPitch, -MaxPitchAbsDeg, +MaxPitchAbsDeg);
	}
	
	// Apply pitch rotation based on speed mode
	if (!bUseDegPerSecSpeed)
	{
		// Use RInterpTo speeds (original behavior)
		const FRotator CurrentPitchRel = PivotGun->GetRelativeRotation();
		const FRotator DesiredPitchRel(DesiredPitch, 0.f, 0.f);
		const FRotator NewPitchRel = FMath::RInterpTo(CurrentPitchRel, DesiredPitchRel, Dt, PitchInterpSpeed);
		PivotGun->SetRelativeRotation(FRotator(NewPitchRel.Pitch, 0.f, 0.f));
	}
	else
	{
		// Use deg/sec stepping
		const float CurrentPitch = FRotator::NormalizeAxis(PivotGun->GetRelativeRotation().Pitch);
		const float Delta = FMath::FindDeltaAngleDegrees(CurrentPitch, DesiredPitch);
		const float Step = FMath::Clamp(Delta, -PitchDegPerSec * Dt, +PitchDegPerSec * Dt);
		const float NewPitch = FRotator::NormalizeAxis(CurrentPitch + Step);
		PivotGun->SetRelativeRotation(FRotator(NewPitch, 0.f, 0.f));
	}
	
	// Debug visualization
	if (bDebugAim && GetWorld())
	{
		// Update debug accumulator for on-screen print
		DebugAccumTime += Dt;
		const bool bShouldPrint = DebugAccumTime >= 0.25f;
		if (bShouldPrint)
		{
			DebugAccumTime = 0.0f;
		}
		
		// Ensure debug draw duration is never negative
		const float SafeDebugDrawDuration = FMath::Max(0.f, DebugDrawDuration);
		
		// Draw line from muzzle to target (red)
		DrawDebugLine(GetWorld(), Start, AimLoc, FColor::Red, false, SafeDebugDrawDuration, 0, 2.0f);
		DrawDebugSphere(GetWorld(), AimLoc, 25.f, 8, FColor::Green, false, SafeDebugDrawDuration);
		
		// Draw forward vector arrow (blue)
		const FVector MuzzleForward = Muzzle->GetForwardVector();
		DrawDebugDirectionalArrow(GetWorld(), Start, Start + MuzzleForward * 200, 20, FColor::Blue, false, SafeDebugDrawDuration, 0, 2.0f);
		
		// Draw desired aim direction arrow (green)
		DrawDebugDirectionalArrow(GetWorld(), Start, Start + AimDirWorld * 200, 20, FColor::Green, false, SafeDebugDrawDuration, 0, 2.0f);
		
		// Draw PivotBase forward arrow (yellow)
		const FVector BaseLoc = PivotBase->GetComponentLocation();
		const FVector BaseForward = PivotBase->GetForwardVector();
		DrawDebugDirectionalArrow(GetWorld(), BaseLoc, BaseLoc + BaseForward * 150.f, 20, FColor::Yellow, false, SafeDebugDrawDuration, 0, 2.0f);
		
		// On-screen print once per 0.25s
		if (bShouldPrint)
		{
			const float CurrentYaw = FRotator::NormalizeAxis(PivotBase->GetComponentRotation().Yaw);
			const float CurrentPitch = FRotator::NormalizeAxis(PivotGun->GetRelativeRotation().Pitch);
			const float DesiredYawFinal = FRotator::NormalizeAxis(DesiredYaw);
			const float DesiredPitchFinal = FRotator::NormalizeAxis(DesiredPitch);
			
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, 
				FString::Printf(TEXT("Yaw: %.1f -> %.1f | Pitch: %.1f -> %.1f"), 
					CurrentYaw, DesiredYawFinal, CurrentPitch, DesiredPitchFinal));
		}
	}
}
