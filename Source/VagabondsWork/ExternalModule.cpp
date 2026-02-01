// Fill out your copyright notice in the Description page of Project Settings.

#include "ExternalModule.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

// Sets default values
AExternalModule::AExternalModule()
{
	// Set this actor to not call Tick() every frame to improve performance
	PrimaryActorTick.bCanEverTick = false;

	// Create core component hierarchy
	ModuleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ModuleRoot"));
	SetRootComponent(ModuleRoot);

	BaseYawPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BaseYawPivot"));
	BaseYawPivot->SetupAttachment(ModuleRoot);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(BaseYawPivot);

	GunPitchPivot = CreateDefaultSubobject<USceneComponent>(TEXT("GunPitchPivot"));
	GunPitchPivot->SetupAttachment(BaseYawPivot);

	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(GunPitchPivot);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(GunPitchPivot);

	// Initialize target storage
	TargetActor = nullptr;
	bUseTargetActor = false;
	TargetLocation = FVector::ZeroVector;

	// Initialize aiming configuration
	YawSpeedDegPerSec = 90.0f;
	PitchSpeedDegPerSec = 45.0f;
	MinYawDeg = -180.0f;
	MaxYawDeg = 180.0f;
	MinPitchDeg = -45.0f;
	MaxPitchDeg = 45.0f;
	bDrawAimDebug = false;

	// Initialize auto-aim timer configuration
	bAutoAimTick = false;
	AimUpdateHz = 20.0f;
}

// Called when the game starts or when spawned
void AExternalModule::BeginPlay()
{
	Super::BeginPlay();

	// Set up auto-aim timer if enabled
	if (bAutoAimTick && AimUpdateHz > 0.0f)
	{
		float UpdateInterval = 1.0f / AimUpdateHz;
		GetWorld()->GetTimerManager().SetTimer(AutoAimTimerHandle, this, &AExternalModule::OnAimTimer, UpdateInterval, true);
	}
}

// Called when the actor is being removed from the game
void AExternalModule::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clear the auto-aim timer
	if (AutoAimTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoAimTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

// Private method called by timer
void AExternalModule::OnAimTimer()
{
	AimStep(1.0f / AimUpdateHz);
}

// Blueprint API - Generic implementations that subclasses can override
void AExternalModule::SetTargetActor(AActor* InTarget)
{
	if (InTarget)
	{
		TargetActor = InTarget;
		bUseTargetActor = true;
	}
}

void AExternalModule::SetTargetLocation(FVector InLocation)
{
	TargetLocation = InLocation;
	bUseTargetActor = false;
}

void AExternalModule::ClearTarget()
{
	TargetActor = nullptr;
	bUseTargetActor = false;
}

FVector AExternalModule::GetAimLocation() const
{
	if (bUseTargetActor && TargetActor)
	{
		return TargetActor->GetActorLocation();
	}
	return TargetLocation;
}

FVector AExternalModule::GetMuzzleWorldLocation() const
{
	// Base implementation - return muzzle location if available, otherwise gun pivot location
	if (Muzzle)
	{
		return Muzzle->GetComponentLocation();
	}
	else if (GunPitchPivot)
	{
		return GunPitchPivot->GetComponentLocation();
	}
	return GetActorLocation();
}

FVector AExternalModule::GetMuzzleWorldForward() const
{
	// Base implementation - return muzzle forward vector if available, otherwise gun pivot forward
	if (Muzzle)
	{
		return Muzzle->GetForwardVector();
	}
	else if (GunPitchPivot)
	{
		return GunPitchPivot->GetForwardVector();
	}
	return GetActorForwardVector();
}

void AExternalModule::AimStep(float DeltaTime)
{
	if (DeltaTime <= 0.0f)
	{
		return;
	}

	if (!BaseYawPivot || !GunPitchPivot)
	{
		return;
	}

	// Check if we have a valid target
	bool bHasValidTarget = false;
	FVector TargetLocation = GetAimLocation();
	
	if (bUseTargetActor)
	{
		bHasValidTarget = TargetActor != nullptr;
	}
	else
	{
		bHasValidTarget = true; // Location target is always valid
	}

	if (!bHasValidTarget)
	{
		return;
	}

	// Get aim direction from muzzle to target
	FVector AimDirection = GetAimDirection();
	if (AimDirection.IsNearlyZero())
	{
		return;
	}

	// Convert direction to pivot local space
	FVector LocalDirection = BaseYawPivot->GetComponentTransform().InverseTransformVectorNoScale(AimDirection);

	// Calculate yaw and pitch angles (assuming forward +X, right +Y, up +Z)
	float YawAngle = FMath::Atan2(LocalDirection.Y, LocalDirection.X);
	float PitchAngle = FMath::Atan2(LocalDirection.Z, LocalDirection.X);

	// Convert from radians to degrees
	YawAngle = FMath::RadiansToDegrees(YawAngle);
	PitchAngle = FMath::RadiansToDegrees(PitchAngle);

	// Clamp angles to limits
	YawAngle = FMath::Clamp(YawAngle, MinYawDeg, MaxYawDeg);
	PitchAngle = FMath::Clamp(PitchAngle, MinPitchDeg, MaxPitchDeg);

	// Get current rotations
	FRotator CurrentYawRotation = BaseYawPivot->GetRelativeRotation();
	FRotator CurrentPitchRotation = GunPitchPivot->GetRelativeRotation();

	// Calculate rotation deltas
	float YawDelta = YawAngle - CurrentYawRotation.Yaw;
	float PitchDelta = PitchAngle - CurrentPitchRotation.Pitch;

	// Normalize yaw delta to handle wrapping
	YawDelta = FRotator::NormalizeAxis(YawDelta);

	// Apply rotation limits based on speed
	float MaxYawDelta = YawSpeedDegPerSec * DeltaTime;
	float MaxPitchDelta = PitchSpeedDegPerSec * DeltaTime;

	YawDelta = FMath::Clamp(YawDelta, -MaxYawDelta, MaxYawDelta);
	PitchDelta = FMath::Clamp(PitchDelta, -MaxPitchDelta, MaxPitchDelta);

	// Apply rotations with smooth interpolation
	FRotator NewYawRotation = FMath::RInterpConstantTo(CurrentYawRotation, FRotator(0.0f, YawAngle, 0.0f), DeltaTime, YawSpeedDegPerSec);
	FRotator NewPitchRotation = FMath::RInterpConstantTo(CurrentPitchRotation, FRotator(PitchAngle, 0.0f, 0.0f), DeltaTime, PitchSpeedDegPerSec);

	// Zero-out roll components
	NewYawRotation.Roll = 0.0f;
	NewPitchRotation.Roll = 0.0f;
	NewYawRotation.Pitch = 0.0f;
	NewPitchRotation.Yaw = 0.0f;

	BaseYawPivot->SetRelativeRotation(NewYawRotation);
	GunPitchPivot->SetRelativeRotation(NewPitchRotation);

	// Debug visualization
	if (bDrawAimDebug && GetWorld())
	{
		FVector MuzzleLocation = GetMuzzleWorldLocation();
		DrawDebugLine(GetWorld(), MuzzleLocation, MuzzleLocation + AimDirection * 1000.0f, FColor::Red, false, -1.0f, 0, 2.0f);
		DrawDebugSphere(GetWorld(), TargetLocation, 20.0f, 12, FColor::Green, false, -1.0f, 0, 2.0f);
	}
}

// Private helper function
FVector AExternalModule::GetAimDirection() const
{
	FVector MuzzleLocation = GetMuzzleWorldLocation();
	FVector TargetLocation = GetAimLocation();
	
	FVector Direction = TargetLocation - MuzzleLocation;
	Direction.Normalize();
	
	return Direction;
}
