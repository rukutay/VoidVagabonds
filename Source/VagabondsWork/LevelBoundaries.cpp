// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelBoundaries.h"
#include "NavStaticBig.h"
#include "Sun.h"
#include "EngineUtils.h"
#include "Components/SphereComponent.h"
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
	
}

// Called every frame
void ALevelBoundaries::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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

	while (SpawnedLocations.Num() < TargetCount && Attempts < MaxAttempts)
	{
		++Attempts;
		const FVector Candidate = Center + FMath::VRand() * FMath::FRandRange(0.0f, SpawnRadius);

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

