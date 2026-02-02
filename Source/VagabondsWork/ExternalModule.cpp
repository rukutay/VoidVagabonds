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
	
	// Desired LookAt rotation using BP-style FindLookAtRotation
	const FRotator LookAt = UKismetMathLibrary::FindLookAtRotation(Start, TargetLoc);
	
	// Apply yaw to PivotBase (ONLY yaw)
	const FRotator CurrentYawWorld = PivotBase->GetComponentRotation();
	const FRotator DesiredYawWorld = FRotator(0, LookAt.Yaw, 0);
	const FRotator NewYawWorld = FMath::RInterpTo(CurrentYawWorld, DesiredYawWorld, Dt, YawInterpSpeed);
	PivotBase->SetWorldRotation(FRotator(0, NewYawWorld.Yaw, 0));
	
	// Recompute pitch after yaw is applied
	const FVector Start2 = bUseMuzzleAsStart ? Muzzle->GetComponentLocation() : PivotGun->GetComponentLocation();
	const FRotator LookAt2 = UKismetMathLibrary::FindLookAtRotation(Start2, TargetLoc);
	
	float DesiredPitch = FRotator::NormalizeAxis(LookAt2.Pitch);
	if (bLimitPitch)
	{
		DesiredPitch = FMath::Clamp(DesiredPitch, -MaxPitchAbsDeg, MaxPitchAbsDeg);
	}
	
	const FRotator CurrentPitchRel = PivotGun->GetRelativeRotation();
	const FRotator DesiredPitchRel = FRotator(DesiredPitch, 0, 0);
	const FRotator NewPitchRel = FMath::RInterpTo(CurrentPitchRel, DesiredPitchRel, Dt, PitchInterpSpeed);
	//ТУТ 
	PivotGun->SetRelativeRotation(FRotator(NewPitchRel.Pitch, 0, 0));
	
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
		
		// Draw line from muzzle to target (red)
		DrawDebugLine(GetWorld(), Start2, TargetLoc, FColor::Red, false, DebugDrawDuration, 0, 2.0f);
		
		// Draw forward vector arrow (blue)
		const FVector MuzzleForward = Muzzle->GetForwardVector();
		DrawDebugDirectionalArrow(GetWorld(), Start2, Start2 + MuzzleForward * 200, 20, FColor::Blue, false, DebugDrawDuration, 0, 2.0f);
		
		// Draw desired aim direction arrow (green)
		const FVector DesiredAimDir = (TargetLoc - Start2).GetSafeNormal();
		DrawDebugDirectionalArrow(GetWorld(), Start2, Start2 + DesiredAimDir * 200, 20, FColor::Green, false, DebugDrawDuration, 0, 2.0f);
		
		// On-screen print once per 0.25s
		if (bShouldPrint)
		{
			const float CurrentYaw = FRotator::NormalizeAxis(PivotBase->GetComponentRotation().Yaw);
			const float CurrentPitch = FRotator::NormalizeAxis(PivotGun->GetRelativeRotation().Pitch);
			const float DesiredYaw = FRotator::NormalizeAxis(LookAt.Yaw);
			const float DesiredPitchFinal = FRotator::NormalizeAxis(DesiredPitch);
			
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::White, 
				FString::Printf(TEXT("Yaw: %.1f -> %.1f | Pitch: %.1f -> %.1f"), 
					CurrentYaw, DesiredYaw, CurrentPitch, DesiredPitchFinal));
		}
	}
}
