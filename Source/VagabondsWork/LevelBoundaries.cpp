// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBoundaries.h"
#include "NavStaticBig.h"
#include "Sun.h"
#include "EngineUtils.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
ALevelBoundaries::ALevelBoundaries()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Boudaries = CreateDefaultSubobject<USphereComponent>(TEXT("Boudaries"));
	RootComponent = Boudaries;
	Boudaries->InitSphereRadius(1250000.0f);
	Boudaries->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

// Called when the game starts or when spawned
void ALevelBoundaries::BeginPlay()
{
	Super::BeginPlay();
	StartAtmosphereControl();
}

// Called every frame
void ALevelBoundaries::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ALevelBoundaries::StartAtmosphereControl()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float SafeInterval = FMath::Max(EnvironmentControlIntervalSec, 0.1f);
	GetWorldTimerManager().SetTimer(AtmosphereControlTimerHandle, this, &ALevelBoundaries::HandleAtmosphereControlTick, SafeInterval, true);
	SpawnInitialAtmosphereAtPlayer();
	HandleAtmosphereControlTick();
}

void ALevelBoundaries::HandleAtmosphereControlTick()
{
	AActor* PlayerActor = GetPrimaryPlayerActor();
	if (!PlayerActor)
	{
		return;
	}

	CleanupDestroyedAtmosphereEntries();
	DespawnIrrelevantAtmosphere(PlayerActor->GetActorLocation());

	if (ActiveAtmosphereEntries.Num() >= MaxAtmosphereInstances)
	{
		return;
	}

	if (IsPlayerInsideAnyAtmosphere(PlayerActor->GetActorLocation()))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float CurrentTime = World->GetTimeSeconds();
	if (NextAtmosphereSpawnTime <= 0.0f)
	{
		const float MinInterval = FMath::Max(SpawnMinIntervalSec, 1.0f);
		const float MaxInterval = FMath::Max(SpawnMaxIntervalSec, MinInterval);
		NextAtmosphereSpawnTime = CurrentTime + FMath::FRandRange(MinInterval, MaxInterval);
		return;
	}

	if (CurrentTime < NextAtmosphereSpawnTime)
	{
		return;
	}

	const FVector PlayerVelocity = PlayerActor->GetVelocity();
	TrySpawnAtmosphereForTravel(PlayerActor->GetActorLocation(), PlayerVelocity);

	const float MinInterval = FMath::Max(SpawnMinIntervalSec, 1.0f);
	const float MaxInterval = FMath::Max(SpawnMaxIntervalSec, MinInterval);
	NextAtmosphereSpawnTime = CurrentTime + FMath::FRandRange(MinInterval, MaxInterval);
}

AActor* ALevelBoundaries::GetPrimaryPlayerActor() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return nullptr;
	}

	if (APawn* Pawn = PlayerController->GetPawn())
	{
		return Pawn;
	}

	return PlayerController->GetViewTarget();
}

bool ALevelBoundaries::SpawnInitialAtmosphereAtPlayer()
{
	AActor* PlayerActor = GetPrimaryPlayerActor();
	if (!PlayerActor)
	{
		return false;
	}

	for (const FAtmosphereSpawnConfig& Config : AtmosphereActors)
	{
		if (Config.AtmosphereClass)
		{
			return SpawnAtmosphereAtLocation(Config, PlayerActor->GetActorLocation());
		}
	}

	return false;
}

void ALevelBoundaries::CleanupDestroyedAtmosphereEntries()
{
	for (int32 Index = ActiveAtmosphereEntries.Num() - 1; Index >= 0; --Index)
	{
		if (!ActiveAtmosphereEntries[Index].SpawnedActor.IsValid())
		{
			ActiveAtmosphereEntries.RemoveAt(Index);
		}
	}
}

void ALevelBoundaries::DespawnIrrelevantAtmosphere(const FVector& PlayerLocation)
{
	const float ClampedMultiplier = FMath::Max(DespawnDistanceMultiplier, 0.0f);
	for (int32 Index = ActiveAtmosphereEntries.Num() - 1; Index >= 0; --Index)
	{
		FAtmosphereRuntimeEntry& Entry = ActiveAtmosphereEntries[Index];
		AActor* Actor = Entry.SpawnedActor.Get();
		if (!Actor)
		{
			ActiveAtmosphereEntries.RemoveAt(Index);
			continue;
		}
		const float RelevanceRadius = Entry.EffectiveRadius * ClampedMultiplier;
		const float RelevanceRadiusSq = RelevanceRadius * RelevanceRadius;
		if (FVector::DistSquared(PlayerLocation, Actor->GetActorLocation()) > RelevanceRadiusSq)
		{
			Actor->Destroy();
			ActiveAtmosphereEntries.RemoveAt(Index);
		}
	}
}

bool ALevelBoundaries::IsPlayerInsideAnyAtmosphere(const FVector& PlayerLocation) const
{
	for (const FAtmosphereRuntimeEntry& Entry : ActiveAtmosphereEntries)
	{
		const AActor* Actor = Entry.SpawnedActor.Get();
		if (!Actor)
		{
			continue;
		}
		const float RadiusSq = Entry.EffectiveRadius * Entry.EffectiveRadius;
		if (FVector::DistSquared(PlayerLocation, Actor->GetActorLocation()) <= RadiusSq)
		{
			return true;
		}
	}

	return false;
}

bool ALevelBoundaries::TrySpawnAtmosphereForTravel(const FVector& PlayerLocation, const FVector& PlayerVelocity)
{
	if (AtmosphereActors.Num() == 0 || ActiveAtmosphereEntries.Num() >= MaxAtmosphereInstances)
	{
		return false;
	}

	TArray<int32> ValidIndices;
	ValidIndices.Reserve(AtmosphereActors.Num());
	for (int32 Index = 0; Index < AtmosphereActors.Num(); ++Index)
	{
		if (AtmosphereActors[Index].AtmosphereClass)
		{
			ValidIndices.Add(Index);
		}
	}

	if (ValidIndices.Num() == 0)
	{
		return false;
	}

	const int32 SelectedIndex = ValidIndices[FMath::RandRange(0, ValidIndices.Num() - 1)];
	const FAtmosphereSpawnConfig& Config = AtmosphereActors[SelectedIndex];
	const FVector SpawnLocation = ComputePredictedSpawnLocation(Config, PlayerLocation, PlayerVelocity);
	const float EffectiveRadius = FMath::Max(Config.ExpectedSphereRadius, 100000.0f);
	if (!CanSpawnWithoutSameClassOverlap(Config.AtmosphereClass, SpawnLocation, EffectiveRadius))
	{
		return false;
	}

	return SpawnAtmosphereAtLocation(Config, SpawnLocation);
}

FVector ALevelBoundaries::ComputePredictedSpawnLocation(const FAtmosphereSpawnConfig& Config, const FVector& PlayerLocation, const FVector& PlayerVelocity) const
{
	const float ExpectedRadius = FMath::Max(Config.ExpectedSphereRadius, 100000.0f);
	FVector Direction = PlayerVelocity.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		if (const AActor* PlayerActor = GetPrimaryPlayerActor())
		{
			Direction = PlayerActor->GetActorForwardVector().GetSafeNormal();
		}
	}
	if (Direction.IsNearlyZero())
	{
		Direction = FVector::ForwardVector;
	}

	const FVector Predicted = PlayerLocation + (PlayerVelocity * Config.PredictionTimeSec);
	const float ForwardOffset = ExpectedRadius;
	FVector SpawnLocation = Predicted + (Direction * ForwardOffset);

	if (Config.SpawnJitterRadius > 0.0f)
	{
		const FVector UpAxis = FVector::UpVector;
		FVector RightAxis = FVector::CrossProduct(Direction, UpAxis).GetSafeNormal();
		if (RightAxis.IsNearlyZero())
		{
			RightAxis = FVector::RightVector;
		}
		const FVector UpOrtho = FVector::CrossProduct(Direction, RightAxis).GetSafeNormal();
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const float Radius = FMath::Sqrt(FMath::FRand()) * Config.SpawnJitterRadius;
		const FVector JitterOffset = (RightAxis * FMath::Cos(Angle) + UpOrtho * FMath::Sin(Angle)) * Radius;
		SpawnLocation += JitterOffset;
	}

	return SpawnLocation;
}

bool ALevelBoundaries::CanSpawnWithoutSameClassOverlap(TSubclassOf<AActor> Class, const FVector& CandidateLocation, float CandidateRadius) const
{
	if (!Class)
	{
		return false;
	}

	for (const FAtmosphereRuntimeEntry& Entry : ActiveAtmosphereEntries)
	{
		if (Entry.AtmosphereClass != Class)
		{
			continue;
		}
		const AActor* Actor = Entry.SpawnedActor.Get();
		if (!Actor)
		{
			continue;
		}
		const float CombinedRadius = CandidateRadius + Entry.EffectiveRadius;
		if (FVector::DistSquared(CandidateLocation, Actor->GetActorLocation()) < CombinedRadius * CombinedRadius)
		{
			return false;
		}
	}

	return true;
}

float ALevelBoundaries::ResolveActorSphereRadiusOrDefault(AActor* Actor, float FallbackRadius) const
{
	const float ClampedFallback = FMath::Max(FallbackRadius, 100000.0f);
	if (!Actor)
	{
		return ClampedFallback;
	}

	const USphereComponent* SphereComponent = Cast<USphereComponent>(Actor->GetRootComponent());
	if (!SphereComponent)
	{
		return ClampedFallback;
	}

	const float Radius = SphereComponent->GetScaledSphereRadius();
	return FMath::Max(Radius, 100000.0f);
}

bool ALevelBoundaries::SpawnAtmosphereAtLocation(const FAtmosphereSpawnConfig& Config, const FVector& Location)
{
	UWorld* World = GetWorld();
	if (!World || !Config.AtmosphereClass)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* SpawnedActor = World->SpawnActor<AActor>(Config.AtmosphereClass, Location, FRotator::ZeroRotator, SpawnParams);
	if (!SpawnedActor)
	{
		return false;
	}

	FAtmosphereRuntimeEntry Entry;
	Entry.AtmosphereClass = Config.AtmosphereClass;
	Entry.SpawnedActor = SpawnedActor;
	Entry.EffectiveRadius = ResolveActorSphereRadiusOrDefault(SpawnedActor, Config.ExpectedSphereRadius);
	ActiveAtmosphereEntries.Add(Entry);

	return true;
}

void ALevelBoundaries::NavStaticBig(TSubclassOf<ANavStaticBig> NavStaticBigClass, int32 MinNum, int32 MaxNum, float MinDistance)
{
	if (!Boudaries)
	{
		return;
	}

	if (!NavStaticBigClass)
	{
		return;
	}

	if (MaxNum < MinNum)
	{
		Swap(MaxNum, MinNum);
	}

	const int32 TargetCount = FMath::Max(0, FMath::RandRange(MinNum, MaxNum));
	if (TargetCount == 0)
	{
		return;
	}

	const float SpawnRadius = Boudaries->GetScaledSphereRadius();
	if (SpawnRadius <= 0.0f || MinDistance <= 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector SunLocation = FVector::ZeroVector;
	const ASun* SunActor = Cast<ASun>(UGameplayStatics::GetActorOfClass(World, ASun::StaticClass()));
	const bool bHasSun = (SunActor != nullptr);
	if (bHasSun)
	{
		SunLocation = SunActor->GetActorLocation();
	}

	TArray<FVector> ExistingLocations;
	for (TActorIterator<ANavStaticBig> It(World); It; ++It)
	{
		ExistingLocations.Add(It->GetActorLocation());
	}

	TArray<FVector> SpawnedLocations;
	SpawnedLocations.Reserve(TargetCount);

	const FVector Center = GetActorLocation();
	const float MinDistanceSq = MinDistance * MinDistance;
	const int32 MaxAttempts = TargetCount * 20;
	int32 Attempts = 0;
	FVector PlaneNormal = FVector::UpVector;
	if (bHasSun)
	{
		PlaneNormal = (SunLocation - Center).GetSafeNormal();
		if (PlaneNormal.IsNearlyZero())
		{
			PlaneNormal = FVector::UpVector;
		}
	}
	FVector PlaneAxisA = FVector::ZeroVector;
	FVector PlaneAxisB = FVector::ZeroVector;
	PlaneNormal.FindBestAxisVectors(PlaneAxisA, PlaneAxisB);
	PlaneAxisA = PlaneAxisA.GetSafeNormal();
	PlaneAxisB = PlaneAxisB.GetSafeNormal();
	const float MinTiltDegrees = 3.0f;
	const float MaxTiltDegrees = 7.0f;

	while (SpawnedLocations.Num() < TargetCount && Attempts < MaxAttempts)
	{
		++Attempts;
		const float Radius = FMath::FRandRange(0.0f, SpawnRadius);
		const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
		const FVector PlaneOffset = PlaneAxisA * FMath::Cos(Angle) + PlaneAxisB * FMath::Sin(Angle);
		const float TiltDegrees = FMath::FRandRange(MinTiltDegrees, MaxTiltDegrees);
		const float TiltSign = (FMath::RandBool() ? 1.0f : -1.0f);
		const FVector TiltOffset = PlaneNormal * FMath::Tan(FMath::DegreesToRadians(TiltDegrees)) * TiltSign;
		const FVector Candidate = Center + (PlaneOffset + TiltOffset).GetSafeNormal() * Radius;

		if (bHasSun && FVector::DistSquared(Candidate, SunLocation) < MinDistanceSq)
		{
			continue;
		}

		bool bTooClose = false;
		for (const FVector& Location : ExistingLocations)
		{
			if (FVector::DistSquared(Candidate, Location) < MinDistanceSq)
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			for (const FVector& Location : SpawnedLocations)
			{
				if (FVector::DistSquared(Candidate, Location) < MinDistanceSq)
				{
					bTooClose = true;
					break;
				}
			}
		}

		if (bTooClose)
		{
			continue;
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		World->SpawnActor<ANavStaticBig>(NavStaticBigClass, Candidate, FRotator::ZeroRotator, SpawnParams);
		SpawnedLocations.Add(Candidate);
	}
}

