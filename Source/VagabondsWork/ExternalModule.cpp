// Fill out your copyright notice in the Description page of Project Settings.

#include "ExternalModule.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Projectile.h"
#include "Ship.h"
#include "GameFramework/Actor.h"
#include "MarkerComponent.h"
#include "FactionsSubsystem.h"
#include "EngineUtils.h"

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

	ProjectileSphere = CreateDefaultSubobject<USphereComponent>(TEXT("ProjectileSphere"));
	ProjectileSphere->SetupAttachment(PivotGun);
	ProjectileSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProjectileSphere->SetHiddenInGame(true);

	MarkerComponent = CreateDefaultSubobject<UMarkerComponent>(TEXT("MarkerComponent"));
	MarkerComponent->MarkerType = EMarkerType::Component;
	MarkerComponent->Faction = EFaction::None;

	// Set default config values
	TurretAimMode = ETurretAimMode::Targeted;
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

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick([this]()
		{
			if (const AShip* OwnerShip = Cast<AShip>(GetOwner()))
			{
				EffectiveRange = FMath::Max(0.0f, OwnerShip->EffectiveRange * FMath::Max(0.0f, EffectiveRangeMultiplier));
			}
		});
	}
	
	// Cache initial angles
	InitialBaseYawWorld = FRotator::NormalizeAxis(PivotBase ? PivotBase->GetComponentRotation().Yaw : 0.f);
	InitialGunPitchRel  = FRotator::NormalizeAxis(PivotGun ? PivotGun->GetRelativeRotation().Pitch : 0.f);
	
	// Only start timer if auto aim is enabled and manual aim step is disabled
	if (bAutoAim && !bManualAimStep && AimUpdateHz > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(AimTimerHandle, this, &AExternalModule::UpdateAim, 1.0f / AimUpdateHz, true);
	}
	else
	{
		ReadyToShoot = false;
	}

	UpdateTargetsList();
	if (TargetsListRefreshInterval > 0.0f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			TargetsListRefreshTimerHandle,
			this,
			&AExternalModule::UpdateTargetsList,
			TargetsListRefreshInterval,
			true);
	}
}

// Called when the actor is being removed from the game
void AExternalModule::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	
	GetWorld()->GetTimerManager().ClearTimer(AimTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(BurstTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(TargetsListRefreshTimerHandle);
}

void AExternalModule::SetTargetActor(AActor* NewTarget)
{
	TargetActor = NewTarget;
}

void AExternalModule::ClearTarget()
{
	TargetActor = nullptr;
}

bool AExternalModule::HasVisibilityFromMuzzleToActor(AActor* Candidate) const
{
	if (!GetWorld() || !Muzzle || !IsValid(Candidate)) return false;

	const FVector Start = Muzzle->GetComponentLocation();
	const FVector End = Candidate->GetActorLocation();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ExternalModule_FreeAimVisibility), false);
	Params.AddIgnoredActor(this);
	if (AActor* OwnerActor = GetOwner())
	{
		Params.AddIgnoredActor(OwnerActor);
	}

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	if (!bHit)
	{
		return true;
	}

	AActor* HitActor = Hit.GetActor();
	return HitActor == Candidate || (HitActor && HitActor->IsOwnedBy(Candidate));
}

AActor* AExternalModule::FindClosestVisibleTargetFromMuzzle() const
{
	if (!Muzzle || !GetWorld()) return nullptr;

	const FVector MuzzleLoc = Muzzle->GetComponentLocation();
	const FVector MuzzleForward = Muzzle->GetForwardVector().GetSafeNormal();
	AActor* ClosestTarget = nullptr;
	float BestScore = MAX_flt;

	for (AActor* Candidate : TargetsList)
	{
		if (!IsValid(Candidate)) continue;
		if (Candidate == this || Candidate == GetOwner()) continue;
		if (!HasVisibilityFromMuzzleToActor(Candidate)) continue;

		const FVector ToCandidate = Candidate->GetActorLocation() - MuzzleLoc;
		const float DistCm = ToCandidate.Size();
		if (DistCm <= KINDA_SMALL_NUMBER) continue;

		const FVector ToCandidateDir = ToCandidate / DistCm;
		const float Dot = FMath::Clamp(FVector::DotProduct(MuzzleForward, ToCandidateDir), -1.0f, 1.0f);
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
		float Score = DistCm + (AngleDeg * FMath::Max(0.0f, FreeTargetAngleWeightCmPerDeg));

		if (Candidate == TargetActor)
		{
			Score *= FMath::Clamp(FreeTargetCurrentTargetScoreMultiplier, 0.0f, 1.0f);
		}

		if (Score < BestScore)
		{
			BestScore = Score;
			ClosestTarget = Candidate;
		}
	}

	return ClosestTarget;
}

void AExternalModule::UpdateTargetsList()
{
	TargetsList.Reset();

	UWorld* World = GetWorld();
	if (!World) return;

	const AActor* OwnerActor = GetOwner();
	if (!IsValid(OwnerActor)) return;

	const UMarkerComponent* OwnerMarker = OwnerActor->FindComponentByClass<UMarkerComponent>();
	if (!IsValid(OwnerMarker) && IsValid(MarkerComponent))
	{
		OwnerMarker = MarkerComponent;
	}
	if (!IsValid(OwnerMarker)) return;

	const UFactionsSubsystem* FactionsSubsystem = World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UFactionsSubsystem>() : nullptr;
	if (!IsValid(FactionsSubsystem)) return;

	const FVector Origin = Muzzle ? Muzzle->GetComponentLocation() : GetActorLocation();
	const float MaxDistSq = (EffectiveRange > 0.0f) ? FMath::Square(EffectiveRange) : MAX_flt;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate)) continue;
		if (Candidate == this || Candidate == OwnerActor) continue;

		const UMarkerComponent* CandidateMarker = Candidate->FindComponentByClass<UMarkerComponent>();
		if (!IsValid(CandidateMarker)) continue;

		if (FactionsSubsystem->GetRelation(OwnerMarker->Faction, CandidateMarker->Faction) >= 0)
		{
			continue;
		}

		if (FVector::DistSquared(Origin, Candidate->GetActorLocation()) > MaxDistSq)
		{
			continue;
		}

		TargetsList.Add(Candidate);
	}
}

bool AExternalModule::HasLineOfSightToTarget() const
{
    if (!bUseReadyToShootCheck) return false;
    if (!GetWorld() || !TargetActor || !PivotGun || !Muzzle) return false;

    const FVector MuzzleLoc = Muzzle->GetComponentLocation();

    // --- AimLoc (prediction) ---
    const FVector TargetLoc = TargetActor->GetActorLocation();
    FVector AimLoc = TargetLoc;
    if (bUseLeadPrediction && ProjectileInitialSpeed > 1.f)
    {
        const FVector TargetVel = TargetActor->GetVelocity(); // works for moving actors; fallback is 0
        const float Dist = FVector::Dist(MuzzleLoc, AimLoc);
        const float Time = Dist / ProjectileInitialSpeed;
        AimLoc += TargetVel * Time;
    }

    const float Radius = ProjectileSphere->GetScaledSphereRadius();
	const FVector MuzzleForwardWithRecoil = (Muzzle->GetComponentRotation() + FRotator(CurrentRecoilPitchDeg, CurrentRecoilYawDeg, 0.0f)).Vector().GetSafeNormal();

    FCollisionQueryParams Params(SCENE_QUERY_STAT(ExternalModule_LOS_Sweep), false);
    Params.AddIgnoredActor(this);
    if (AActor* OwnerActor = GetOwner())
    {
        Params.AddIgnoredActor(OwnerActor);
    }

    const FCollisionShape Sphere = FCollisionShape::MakeSphere(Radius);

    auto HasClearShotToLoc = [&](const FVector& ShotLoc) -> bool
    {
        // Optional range gate (cheap)
        if (EffectiveRange > 0.f && FVector::DistSquared(MuzzleLoc, ShotLoc) > FMath::Square(EffectiveRange))
        {
            return false;
        }

        const FVector ToAim = (ShotLoc - MuzzleLoc);
        const float ToAimLenSq = ToAim.SizeSquared();
        if (ToAimLenSq < 1.f) return true;

		// Accuracy + distance + recoil aware acceptance with hysteresis.
        const FVector AimDir = ToAim.GetSafeNormal();
		const float Dot = FMath::Clamp(FVector::DotProduct(MuzzleForwardWithRecoil, AimDir), -1.0f, 1.0f);
        const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(Dot));
        const float ClampedAccuracy = FMath::Clamp(Accuracy, 0.0f, 1.0f);
		float AllowedSpreadDeg = FMath::Lerp(12.0f, 0.5f, ClampedAccuracy);

		const float BonusDistance = FMath::Max(ReadyToShootBonusDistanceCm, 1.0f);
		const float DistAlpha = FMath::Clamp(FMath::Sqrt(ToAimLenSq) / BonusDistance, 0.0f, 1.0f);
		AllowedSpreadDeg += DistAlpha * FMath::Max(0.0f, ReadyToShootDistanceSpreadBonusDeg);

		if (ReadyToShoot)
		{
			AllowedSpreadDeg *= FMath::Max(1.0f, ReadyToShootHysteresisMultiplier);
		}

        if (AngleDeg > AllowedSpreadDeg)
        {
            return false;
        }

        auto SweepBlocked = [&](ECollisionChannel Channel) -> bool
        {
            FHitResult Hit;
            const bool bHit = GetWorld()->SweepSingleByChannel(Hit, MuzzleLoc, ShotLoc, FQuat::Identity, Channel, Sphere, Params);
            if (!bHit) return false; // clear on this channel
            return Hit.GetActor() != TargetActor; // only target is allowed
        };

        if (SweepBlocked(ECC_GameTraceChannel2)) return false;
        if (SweepBlocked(ECC_Pawn))              return false;
        if (SweepBlocked(ECC_PhysicsBody))       return false;

        return true;
    };

    if (HasClearShotToLoc(AimLoc)) return true;
    if (bUseLeadPrediction && !AimLoc.Equals(TargetLoc, 0.01f))
    {
        return HasClearShotToLoc(TargetLoc);
    }

    return false;
}

float AExternalModule::GetProjectileCollisionRadius(TSubclassOf<AProjectile> ProjectileClass) const
{
	if (ProjectileClass)
	{
		const AProjectile* DefaultProjectile = ProjectileClass->GetDefaultObject<AProjectile>();
		if (DefaultProjectile && DefaultProjectile->Collision)
		{
			return DefaultProjectile->Collision->GetScaledSphereRadius();
		}
	}

	return ProjectileSphere ? ProjectileSphere->GetScaledSphereRadius() : 8.0f;
}

float AExternalModule::GetShotInterval(TSubclassOf<AProjectile> ProjectileClass, float ProjectileSpeed) const
{
	const float FireInterval = 1.0f / FMath::Max(FireRate, 0.1f);
	const float Radius = GetProjectileCollisionRadius(ProjectileClass);
	const float Speed = FMath::Max(ProjectileSpeed, 1.0f);
	const float SeparationTime = (Radius * 2.5f) / Speed;
	return FMath::Max(FireInterval, SeparationTime);
}

bool AExternalModule::IsSpawnLocationClear(const FVector& Location, float Radius) const
{
	if (!GetWorld()) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ExternalModule_SpawnCheck), false);
	Params.AddIgnoredActor(this);
	if (AActor* OwnerActor = GetOwner())
	{
		Params.AddIgnoredActor(OwnerActor);
	}

	const FCollisionShape Shape = FCollisionShape::MakeSphere(Radius);
	if (GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_WorldStatic, Shape, Params)) return false;
	if (GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_WorldDynamic, Shape, Params)) return false;
	if (GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_Pawn, Shape, Params)) return false;
	if (GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_PhysicsBody, Shape, Params)) return false;
	if (GetWorld()->OverlapAnyTestByChannel(Location, FQuat::Identity, ECC_GameTraceChannel2, Shape, Params)) return false;

	return true;
}

bool AExternalModule::TrySpawnProjectile(TSubclassOf<AProjectile> ProjectileClass, float ProjectileSpeed, float LifeSpan, float DamageAmount)
{
	if (!GetWorld() || !ProjectileClass || !Muzzle) return false;

	const float Radius = GetProjectileCollisionRadius(ProjectileClass);
	const FVector MuzzleLoc = Muzzle->GetComponentLocation();
	const float ClampedAccuracy = FMath::Clamp(Accuracy, 0.0f, 1.0f);
	const float Inaccuracy = 1.0f - ClampedAccuracy;
	const float Now = GetWorld()->GetTimeSeconds();
	const float DtSinceLastShot = (LastShotTime >= 0.0f) ? FMath::Max(0.0f, Now - LastShotTime) : 0.0f;

	if (DtSinceLastShot > 0.0f && RecoilRecoveryDegPerSec > 0.0f)
	{
		CurrentRecoilYawDeg = FMath::FInterpConstantTo(CurrentRecoilYawDeg, 0.0f, DtSinceLastShot, RecoilRecoveryDegPerSec);
		CurrentRecoilPitchDeg = FMath::FInterpConstantTo(CurrentRecoilPitchDeg, 0.0f, DtSinceLastShot, RecoilRecoveryDegPerSec);
	}

	FRotator SpawnRotation = Muzzle->GetComponentRotation();
	if (Inaccuracy > 0.0f)
	{
		const float ShotKickBaseDeg = RecoilPerShotDeg * Inaccuracy;
		const float YawKick = FMath::FRandRange(-1.0f, 1.0f) * ShotKickBaseDeg * RecoilRandomYawScale;
		const float PitchKick = FMath::FRandRange(0.35f, 1.0f) * ShotKickBaseDeg * RecoilRandomPitchScale;

		CurrentRecoilYawDeg += YawKick;
		CurrentRecoilPitchDeg += PitchKick;

		const float MaxAccumYawDeg = FMath::Lerp(0.0f, 25.0f, Inaccuracy);
		const float MaxAccumPitchDeg = FMath::Lerp(0.0f, 35.0f, Inaccuracy);
		CurrentRecoilYawDeg = FMath::Clamp(CurrentRecoilYawDeg, -MaxAccumYawDeg, MaxAccumYawDeg);
		CurrentRecoilPitchDeg = FMath::Clamp(CurrentRecoilPitchDeg, -MaxAccumPitchDeg, MaxAccumPitchDeg);

		SpawnRotation += FRotator(CurrentRecoilPitchDeg, CurrentRecoilYawDeg, 0.0f);
	}
	LastShotTime = Now;

	const FVector SpawnForward = SpawnRotation.Vector();
	const FQuat SpawnQuat = SpawnRotation.Quaternion();
	const int32 MaxAttempts = 4;
	const float Step = FMath::Max(Radius * 1.5f, 4.0f);

	FVector SpawnLoc = MuzzleLoc;
	bool bFoundClear = false;
	for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
	{
		if (IsSpawnLocationClear(SpawnLoc, Radius))
		{
			bFoundClear = true;
			break;
		}
		SpawnLoc += SpawnForward * Step;
	}

/* 	bFoundClear = true;
	SpawnLoc += Forward * Step; */

	if (!bFoundClear) return false;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = GetInstigator();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FTransform SpawnTransform(SpawnQuat, SpawnLoc);
	AProjectile* Projectile = GetWorld()->SpawnActorDeferred<AProjectile>(ProjectileClass, SpawnTransform, SpawnParams.Owner, SpawnParams.Instigator, SpawnParams.SpawnCollisionHandlingOverride);
	if (!Projectile) return false;

	Projectile->SetOwner(GetOwner());
	if (Projectile->Movement)
	{
		Projectile->Movement->InitialSpeed = ProjectileSpeed;
		Projectile->Movement->MaxSpeed = ProjectileSpeed;
	}
	Projectile->DamageAmount = DamageAmount;
	Projectile->FinishSpawning(SpawnTransform);
	if (LifeSpan > 0.0f)
	{
		Projectile->SetLifeSpan(LifeSpan);
	}

	return true;
}

void AExternalModule::HandleSemiAutoShot()
{
	if (BurstShotsLeft <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimerHandle);
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextAllowedShotTime) return;

	if (!TrySpawnProjectile(BurstProjectileClass, BurstProjectileSpeed, BurstLifeSpan, BurstDamageAmount))
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimerHandle);
		BurstShotsLeft = 0;
		return;
	}

	BurstShotsLeft--;
	if (BurstShotsLeft <= 0)
	{
		NextAllowedShotTime = Now + BurstInterval + BurstDelay;
		GetWorld()->GetTimerManager().ClearTimer(BurstTimerHandle);
		return;
	}

	NextAllowedShotTime = Now + BurstInterval;
	if (BurstShotsLeft <= 0)
	{
		GetWorld()->GetTimerManager().ClearTimer(BurstTimerHandle);
	}
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
	const float Dt = (AimUpdateHz > 0) ? (1.f / AimUpdateHz) : 0.f;
	AimStep(Dt);
}

void AExternalModule::AimStep(float Dt)
{
	if (!PivotBase || !PivotGun || !Muzzle || Dt <= 0) return;

	if (TurretAimMode == ETurretAimMode::Free)
	{
		AActor* BestTarget = FindClosestVisibleTargetFromMuzzle();
		if (BestTarget != TargetActor)
		{
			const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
			const bool bHasCurrentTarget = IsValid(TargetActor);
			const bool bWithinSwitchCooldown = bHasCurrentTarget
				&& FreeTargetSwitchCooldown > 0.0f
				&& LastFreeTargetSwitchTime >= 0.0f
				&& (Now - LastFreeTargetSwitchTime) < FreeTargetSwitchCooldown;

			if (!bWithinSwitchCooldown)
			{
				TargetActor = BestTarget;
				LastFreeTargetSwitchTime = Now;
			}
		}
	}

	if (!TargetActor)
	{
		ReadyToShoot = false;
		if (!bUseDegPerSecSpeed)
		{
			const FRotator CurrentYawRel = PivotBase->GetRelativeRotation();
			const FRotator NewYawRel = FMath::RInterpTo(CurrentYawRel, FRotator::ZeroRotator, Dt, YawInterpSpeed);
			PivotBase->SetRelativeRotation(FRotator(0.f, NewYawRel.Yaw, 0.f));

			const FRotator CurrentPitchRel = PivotGun->GetRelativeRotation();
			const FRotator NewPitchRel = FMath::RInterpTo(CurrentPitchRel, FRotator::ZeroRotator, Dt, PitchInterpSpeed);
			PivotGun->SetRelativeRotation(FRotator(NewPitchRel.Pitch, 0.f, 0.f));
		}
		else
		{
			const float CurrentYaw = FRotator::NormalizeAxis(PivotBase->GetRelativeRotation().Yaw);
			const float YawDelta = FMath::FindDeltaAngleDegrees(CurrentYaw, 0.f);
			const float YawStep = FMath::Clamp(YawDelta, -YawDegPerSec * Dt, +YawDegPerSec * Dt);
			const float NewYaw = FRotator::NormalizeAxis(CurrentYaw + YawStep);
			PivotBase->SetRelativeRotation(FRotator(0.f, NewYaw, 0.f));

			const float CurrentPitch = FRotator::NormalizeAxis(PivotGun->GetRelativeRotation().Pitch);
			const float PitchDelta = FMath::FindDeltaAngleDegrees(CurrentPitch, 0.f);
			const float PitchStep = FMath::Clamp(PitchDelta, -PitchDegPerSec * Dt, +PitchDegPerSec * Dt);
			const float NewPitch = FRotator::NormalizeAxis(CurrentPitch + PitchStep);
			PivotGun->SetRelativeRotation(FRotator(NewPitch, 0.f, 0.f));
		}
		return;
	}

	ReadyToShoot = HasLineOfSightToTarget();

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

void AExternalModule::Shoot(TSubclassOf<AProjectile> ProjectileClass, float ProjectileSpeed, float LifeSpan, float DamageAmount, float Delay)
{
	if (!ProjectileClass || !GetWorld()) return;
	ProjectileInitialSpeed = FMath::Max(ProjectileSpeed, 1.0f);
	ReadyToShoot = HasLineOfSightToTarget();
	if (bUseReadyToShootCheck && !ReadyToShoot) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextAllowedShotTime) return;

	const float Interval = GetShotInterval(ProjectileClass, ProjectileSpeed);
	if (FireMode == EExternalModuleFireMode::SemiAuto)
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(BurstTimerHandle) || BurstShotsLeft > 0)
		{
			return;
		}

		BurstProjectileClass = ProjectileClass;
		BurstProjectileSpeed = ProjectileSpeed;
		BurstLifeSpan = LifeSpan;
		BurstDamageAmount = DamageAmount;
		BurstDelay = FMath::Max(Delay, ShootDelay);
		BurstInterval = Interval;
		BurstShotsLeft = FMath::Max(SemiAutoShootsNumber, 1);

		HandleSemiAutoShot();
		if (BurstShotsLeft > 0)
		{
			GetWorld()->GetTimerManager().SetTimer(BurstTimerHandle, this, &AExternalModule::HandleSemiAutoShot, BurstInterval, true);
		}
		return;
	}

	NextAllowedShotTime = Now + Interval;
	TrySpawnProjectile(ProjectileClass, ProjectileInitialSpeed, LifeSpan, DamageAmount);
}
