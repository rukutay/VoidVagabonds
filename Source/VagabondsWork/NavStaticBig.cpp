// Fill out your copyright notice in the Description page of Project Settings.


#include "NavStaticBig.h"
#include "GameFramework/PlayerController.h"

// Sets default values
ANavStaticBig::ANavStaticBig()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	SetRootComponent(BodyMesh);

	PlaneRadius = CreateDefaultSubobject<USphereComponent>(TEXT("PlaneRadius"));
	PlaneRadius->SetupAttachment(BodyMesh);

	FieldSpline = CreateDefaultSubobject<USplineComponent>(TEXT("FieldSpline"));
	FieldSpline->SetupAttachment(BodyMesh);

	AsteroidHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidHISM"));
	AsteroidHISM->SetupAttachment(BodyMesh);

	AsteroidMidHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidMidHISM"));
	AsteroidMidHISM->SetupAttachment(BodyMesh);

	AsteroidFarHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidFarHISM"));
	AsteroidFarHISM->SetupAttachment(BodyMesh);

	PlaneRadius->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidMidHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidFarHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BodyMesh->LightingChannels.bChannel0 = true;
	BodyMesh->LightingChannels.bChannel1 = false;
	BodyMesh->LightingChannels.bChannel2 = false;
}

void ANavStaticBig::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (FieldSpline && FieldSpline->GetNumberOfSplinePoints() == 0)
	{
		BuildCircularSpline();
	}
}

// Called when the game starts or when spawned
void ANavStaticBig::BeginPlay()
{
	Super::BeginPlay();

	if (bEnableStreaming && StreamingUpdateInterval > 0.0f)
	{
		UpdateAsteroidStreaming();
		GetWorldTimerManager().SetTimer(StreamingTimerHandle, this, &ANavStaticBig::UpdateAsteroidStreaming, StreamingUpdateInterval, true);
	}
}

void ANavStaticBig::BuildCircularSpline(float Radius, int32 NumPoints)
{
	if (!FieldSpline)
	{
		return;
	}

	float EffectiveRadius = Radius;
	if (EffectiveRadius <= 0.0f)
	{
		if (SplineRadiusOverride > 0.0f)
		{
			EffectiveRadius = SplineRadiusOverride;
		}
		else if (PlaneRadius)
		{
			EffectiveRadius = PlaneRadius->GetScaledSphereRadius() * 1.5f;
		}
	}

	FieldSpline->ClearSplinePoints(false);

	const int32 SafePoints = FMath::Max(NumPoints, 3);
	for (int32 PointIndex = 0; PointIndex < SafePoints; ++PointIndex)
	{
		const float Angle = (2.0f * PI * static_cast<float>(PointIndex)) / static_cast<float>(SafePoints);
		const FVector LocalPosition(EffectiveRadius * FMath::Cos(Angle), EffectiveRadius * FMath::Sin(Angle), 0.0f);
		FieldSpline->AddSplinePoint(LocalPosition, ESplineCoordinateSpace::Local, false);
	}

	FieldSpline->SetClosedLoop(true, false);
	FieldSpline->UpdateSpline();
}

void ANavStaticBig::GenerateAsteroidField()
{
	if (bEnableStreaming)
	{
		UpdateAsteroidStreaming();
		return;
	}

	if (!FieldSpline || !AsteroidHISM)
	{
		return;
	}

	if (FieldSpline->GetNumberOfSplinePoints() < 2 || FieldSpline->GetSplineLength() <= 0.0f)
	{
		return;
	}

	if (AsteroidMeshes.Num() == 0)
	{
		return;
	}

	AsteroidHISM->ClearInstances();
	AsteroidHISM->SetStaticMesh(AsteroidMeshes[0]);
	if (AsteroidMidHISM)
	{
		AsteroidMidHISM->ClearInstances();
		AsteroidMidHISM->SetStaticMesh(MidAsteroidMesh ? MidAsteroidMesh : AsteroidMeshes[0]);
	}
	if (AsteroidFarHISM)
	{
		AsteroidFarHISM->ClearInstances();
		AsteroidFarHISM->SetStaticMesh(FarAsteroidMesh ? FarAsteroidMesh : AsteroidMeshes[0]);
	}

	const float SafeDensity = FMath::Max(DensityPer1000uu, 0.001f);
	const float Step = 1000.0f / SafeDensity;
	const float SplineLength = FieldSpline->GetSplineLength();
	const int32 NumSteps = FMath::FloorToInt(SplineLength / Step);
	const float HalfWidth = FieldWidth * 0.5f;
	const float HalfHeight = FieldHeight * 0.5f;
	int32 InstanceLimit = FMath::Max(MaxAsteroidInstances, 0);

#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		InstanceLimit = FMath::Min(InstanceLimit, FMath::Max(PreviewMaxInstancesEditor, 0));
	}
#endif
	if (InstanceLimit == 0)
	{
		return;
	}

	FRandomStream RandomStream(Seed);
	int32 InstancesAdded = 0;

	for (int32 StepIndex = 0; StepIndex <= NumSteps; ++StepIndex)
	{
		if (InstancesAdded >= InstanceLimit)
		{
			break;
		}

		const float Distance = FMath::Min(static_cast<float>(StepIndex) * Step, SplineLength);
		const FVector Location = FieldSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		const FVector Tangent = FieldSpline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		const FVector Forward = Tangent.GetSafeNormal();
		const FMatrix Frame = FRotationMatrix::MakeFromX(Forward);
		const FVector Right = Frame.GetScaledAxis(EAxis::Y);
		const FVector Up = Frame.GetScaledAxis(EAxis::Z);

		const float OffsetRight = RandomStream.FRandRange(-HalfWidth, HalfWidth);
		const float OffsetUp = RandomStream.FRandRange(-HalfHeight, HalfHeight);
		const FVector InstanceLocation = Location + (Right * OffsetRight) + (Up * OffsetUp);
		const FRotator InstanceRotation(
			RandomStream.FRandRange(0.0f, 360.0f),
			RandomStream.FRandRange(0.0f, 360.0f),
			RandomStream.FRandRange(0.0f, 360.0f));
		const float InstanceScale = RandomStream.FRandRange(MinAsteroidScale, MaxAsteroidScale);
		const FTransform InstanceTransform(InstanceRotation, InstanceLocation, FVector(InstanceScale));

		AsteroidHISM->AddInstance(InstanceTransform, true);
		++InstancesAdded;
	}
}

void ANavStaticBig::UpdateAsteroidStreaming()
{
	if (!FieldSpline || !AsteroidHISM || !AsteroidMidHISM || !AsteroidFarHISM)
	{
		return;
	}

	if (FieldSpline->GetNumberOfSplinePoints() < 2 || FieldSpline->GetSplineLength() <= 0.0f)
	{
		return;
	}

	if (AsteroidMeshes.Num() == 0)
	{
		return;
	}

	const FVector ViewLocation = GetStreamingViewLocation();
	const float SplineLength = FieldSpline->GetSplineLength();
	const float EffectiveStreamingRadius = StreamingRadius > 0.0f ? StreamingRadius : SplineLength;
	const float ClampedMidStart = FMath::Max(MidRangeStart, 0.0f);
	const float ClampedMidEnd = FMath::Max(MidRangeEnd, ClampedMidStart);
	const float ClampedFarStart = FMath::Max(FarRangeStart, ClampedMidEnd);
	const float ClampedFarEnd = FMath::Max(FarRangeEnd, ClampedFarStart);
	const float NearDensity = FMath::Max(DensityPer1000uu, 0.001f);
	const float MidDensity = FMath::Max(MidDensityPer1000uu, 0.001f);
	const float FarDensity = FMath::Max(FarDensityPer1000uu, 0.001f);
	const float SeedStep = 1000.0f / FMath::Max3(NearDensity, MidDensity, FarDensity);
	const float HalfWidth = FieldWidth * 0.5f;
	const float HalfHeight = FieldHeight * 0.5f;
	int32 InstanceLimit = FMath::Max(MaxAsteroidInstances, 0);
	int32 UpdateLimit = InstanceLimit;
	int32 NearLimit = FMath::Max(MaxNearInstances, 0);
	int32 MidLimit = FMath::Max(MaxMidInstances, 0);
	int32 FarLimit = FMath::Max(MaxFarInstances, 0);

#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		InstanceLimit = FMath::Min(InstanceLimit, FMath::Max(PreviewMaxInstancesEditor, 0));
		UpdateLimit = InstanceLimit;
		NearLimit = FMath::Min(NearLimit, InstanceLimit);
		MidLimit = FMath::Min(MidLimit, InstanceLimit);
		FarLimit = FMath::Min(FarLimit, InstanceLimit);
	}
#endif

	if (MaxInstancesPerUpdate > 0)
	{
		UpdateLimit = FMath::Min(UpdateLimit, MaxInstancesPerUpdate);
	}
	NearLimit = FMath::Min(NearLimit, InstanceLimit);
	MidLimit = FMath::Min(MidLimit, InstanceLimit);
	FarLimit = FMath::Min(FarLimit, InstanceLimit);

	if (InstanceLimit == 0 || UpdateLimit == 0)
	{
		AsteroidHISM->ClearInstances();
		AsteroidMidHISM->ClearInstances();
		AsteroidFarHISM->ClearInstances();
		return;
	}

	const float InputKey = FieldSpline->FindInputKeyClosestToWorldLocation(ViewLocation);
	const float CenterDistance = FieldSpline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
	const float RawStart = CenterDistance - EffectiveStreamingRadius;
	const float RawEnd = CenterDistance + EffectiveStreamingRadius;
	const bool bClosedLoop = FieldSpline->IsClosedLoop();

	AsteroidHISM->ClearInstances();
	AsteroidMidHISM->ClearInstances();
	AsteroidFarHISM->ClearInstances();
	AsteroidHISM->SetStaticMesh(AsteroidMeshes[0]);
	AsteroidMidHISM->SetStaticMesh(MidAsteroidMesh ? MidAsteroidMesh : AsteroidMeshes[0]);
	AsteroidFarHISM->SetStaticMesh(FarAsteroidMesh ? FarAsteroidMesh : AsteroidMeshes[0]);

	int32 InstancesAdded = 0;
	int32 NearAdded = 0;
	int32 MidAdded = 0;
	int32 FarAdded = 0;
	auto SpawnRange = [&](float StartDistance, float EndDistance)
	{
		float Distance = StartDistance;
		while (Distance <= EndDistance && InstancesAdded < InstanceLimit && InstancesAdded < UpdateLimit)
		{
			const FVector Location = FieldSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			const float DistanceToView = FVector::Dist(Location, ViewLocation);
			float Density = 0.0f;
			UHierarchicalInstancedStaticMeshComponent* TargetHISM = nullptr;
			if (DistanceToView <= ClampedMidStart)
			{
				if (NearAdded < NearLimit)
				{
					Density = NearDensity;
					TargetHISM = AsteroidHISM;
				}
			}
			else if (DistanceToView <= ClampedMidEnd)
			{
				if (MidAdded < MidLimit)
				{
					Density = MidDensity;
					TargetHISM = AsteroidMidHISM;
				}
			}
			else if (DistanceToView <= ClampedFarEnd)
			{
				if (FarAdded < FarLimit)
				{
					Density = FarDensity;
					TargetHISM = AsteroidFarHISM;
				}
			}

			const float Step = (Density > 0.0f) ? (1000.0f / Density) : (1000.0f / NearDensity);
			const FVector Tangent = FieldSpline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			const FVector Forward = Tangent.GetSafeNormal();
			const FMatrix Frame = FRotationMatrix::MakeFromX(Forward);
			const FVector Right = Frame.GetScaledAxis(EAxis::Y);
			const FVector Up = Frame.GetScaledAxis(EAxis::Z);

			if (Density > 0.0f && TargetHISM)
			{
				const int32 StepIndex = FMath::FloorToInt(Distance / SeedStep);
				FRandomStream LocalStream(Seed + StepIndex * 31);
				const float OffsetRight = LocalStream.FRandRange(-HalfWidth, HalfWidth);
				const float OffsetUp = LocalStream.FRandRange(-HalfHeight, HalfHeight);
				const FVector InstanceLocation = Location + (Right * OffsetRight) + (Up * OffsetUp);
				const FRotator InstanceRotation(
					LocalStream.FRandRange(0.0f, 360.0f),
					LocalStream.FRandRange(0.0f, 360.0f),
					LocalStream.FRandRange(0.0f, 360.0f));
				const float InstanceScale = LocalStream.FRandRange(MinAsteroidScale, MaxAsteroidScale);
				const FTransform InstanceTransform(InstanceRotation, InstanceLocation, FVector(InstanceScale));

				TargetHISM->AddInstance(InstanceTransform, true);
				++InstancesAdded;
				if (TargetHISM == AsteroidHISM)
				{
					++NearAdded;
				}
				else if (TargetHISM == AsteroidMidHISM)
				{
					++MidAdded;
				}
				else if (TargetHISM == AsteroidFarHISM)
				{
					++FarAdded;
				}
			}

			Distance += Step;
		}
	};

	if (bClosedLoop && (RawStart < 0.0f || RawEnd > SplineLength))
	{
		if (RawStart < 0.0f)
		{
			SpawnRange(0.0f, FMath::Min(RawEnd, SplineLength));
			if (InstancesAdded < InstanceLimit && InstancesAdded < UpdateLimit)
			{
				SpawnRange(SplineLength + RawStart, SplineLength);
			}
		}
		else
		{
			SpawnRange(RawStart, SplineLength);
			if (InstancesAdded < InstanceLimit && InstancesAdded < UpdateLimit)
			{
				SpawnRange(0.0f, RawEnd - SplineLength);
			}
		}
	}
	else
	{
		const float StartDistance = FMath::Clamp(RawStart, 0.0f, SplineLength);
		const float EndDistance = FMath::Clamp(RawEnd, 0.0f, SplineLength);
		SpawnRange(StartDistance, EndDistance);
	}
}

FVector ANavStaticBig::GetStreamingViewLocation() const
{
	if (!GetWorld())
	{
		return GetActorLocation();
	}

	if (const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (const AActor* ViewTarget = PlayerController->GetViewTarget())
		{
			return ViewTarget->GetActorLocation();
		}
	}

	return GetActorLocation();
}

