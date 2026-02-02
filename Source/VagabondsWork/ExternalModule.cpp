// Fill out your copyright notice in the Description page of Project Settings.

#include "ExternalModule.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

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

	const FVector TargetLoc = TargetActor->GetActorLocation();
	
	// Determine Start point
	const FVector Start = bUseMuzzleAsStart ? Muzzle->GetComponentLocation() : PivotGun->GetComponentLocation();
	
	// Compute AimDirWorld
	const FVector AimDirWorld = (TargetLoc - Start).GetSafeNormal();
	
	// Get parent transform for PivotBase
	USceneComponent* BaseParent = PivotBase->GetAttachParent();
	if (!BaseParent) return;
	
	// Convert direction into BaseParent local space
	const FVector AimDirParent = BaseParent->GetComponentTransform().InverseTransformVectorNoScale(AimDirWorld);
	
	// Compute desired yaw (forward +X, right +Y)
	const float DesiredYaw = FRotator::NormalizeAxis(FMath::RadiansToDegrees(FMath::Atan2(AimDirParent.Y, AimDirParent.X)));
	
	// Interp relative yaw only
	const FRotator CurrentYawRel = PivotBase->GetRelativeRotation();
	const FRotator DesiredYawRel(0.f, DesiredYaw, 0.f);
	const FRotator NewYawRel = FMath::RInterpTo(CurrentYawRel, DesiredYawRel, Dt, YawInterpSpeed);
	
	// Apply
	PivotBase->SetRelativeRotation(FRotator(0.f, NewYawRel.Yaw, 0.f));
	
	// Recompute pitch after yaw is applied
	// Convert AimDirWorld into PivotBase local space
	const FVector AimDirBase = PivotBase->GetComponentTransform().InverseTransformVectorNoScale(AimDirWorld);
	
	// Compute horizontal magnitude
	const float Horizontal = FMath::Sqrt(AimDirBase.X*AimDirBase.X + AimDirBase.Y*AimDirBase.Y);
	if (Horizontal < 1e-4f) return;
	
	// DesiredPitch (degrees)
	float DesiredPitch = FRotator::NormalizeAxis(FMath::RadiansToDegrees(FMath::Atan2(AimDirBase.Z, Horizontal)));
	
	// Apply pitch clamp if enabled
	if (bLimitPitch)
		DesiredPitch = FMath::Clamp(DesiredPitch, -MaxPitchAbsDeg, +MaxPitchAbsDeg);
	
	// Interp relative pitch only
	const FRotator CurrentPitchRel = PivotGun->GetRelativeRotation();
	const FRotator DesiredPitchRel(DesiredPitch, 0.f, 0.f);
	const FRotator NewPitchRel = FMath::RInterpTo(CurrentPitchRel, DesiredPitchRel, Dt, PitchInterpSpeed);
	
	// Apply
	PivotGun->SetRelativeRotation(FRotator(NewPitchRel.Pitch, 0.f, 0.f));
	
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
		DrawDebugLine(GetWorld(), Start, TargetLoc, FColor::Red, false, SafeDebugDrawDuration, 0, 2.0f);
		
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
