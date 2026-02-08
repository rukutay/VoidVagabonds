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

	AsteroidHISMAlt = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidHISMAlt"));
	AsteroidHISMAlt->SetupAttachment(BodyMesh);

	AsteroidMidHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidMidHISM"));
	AsteroidMidHISM->SetupAttachment(BodyMesh);

	AsteroidMidHISMAlt = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidMidHISMAlt"));
	AsteroidMidHISMAlt->SetupAttachment(BodyMesh);

	AsteroidFarHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidFarHISM"));
	AsteroidFarHISM->SetupAttachment(BodyMesh);

	AsteroidFarHISMAlt = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidFarHISMAlt"));
	AsteroidFarHISMAlt->SetupAttachment(BodyMesh);

	PlaneRadius->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidHISMAlt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidMidHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidMidHISMAlt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidFarHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidFarHISMAlt->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidHISM->SetNotifyRigidBodyCollision(true);
	AsteroidHISMAlt->SetNotifyRigidBodyCollision(true);
	AsteroidHISMAlt->SetVisibility(false, true);
	AsteroidMidHISMAlt->SetVisibility(false, true);
	AsteroidFarHISMAlt->SetVisibility(false, true);

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

	if (AsteroidHISM)
	{
		AsteroidHISM->OnComponentHit.AddDynamic(this, &ANavStaticBig::OnAsteroidHit);
	}
	if (AsteroidHISMAlt)
	{
		AsteroidHISMAlt->OnComponentHit.AddDynamic(this, &ANavStaticBig::OnAsteroidHit);
	}

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

	const float ClampedJitterMin = FMath::Max(StepJitterMin, 0.05f);
	const float ClampedJitterMax = FMath::Max(StepJitterMax, ClampedJitterMin);

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

		const float Jitter = RandomStream.FRandRange(ClampedJitterMin, ClampedJitterMax);
		const float Distance = FMath::Min(static_cast<float>(StepIndex) * Step * Jitter, SplineLength);
		const FVector Location = FieldSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		const FVector Tangent = FieldSpline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
		const FVector Forward = Tangent.GetSafeNormal();
		const FMatrix Frame = FRotationMatrix::MakeFromX(Forward);
		const FVector Right = Frame.GetScaledAxis(EAxis::Y);
		const FVector Up = Frame.GetScaledAxis(EAxis::Z);

		if (RandomStream.FRand() > NearSpawnProbability)
		{
			continue;
		}

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

	const float ClampedJitterMin = FMath::Max(StepJitterMin, 0.05f);
	const float ClampedJitterMax = FMath::Max(StepJitterMax, ClampedJitterMin);
	const float ClampedNearSpawn = FMath::Clamp(NearSpawnProbability, 0.0f, 1.0f);
	const float ClampedMidSpawn = FMath::Clamp(MidSpawnProbability, 0.0f, 1.0f);
	const float ClampedFarSpawn = FMath::Clamp(FarSpawnProbability, 0.0f, 1.0f);
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
	const float AdjustedMidStart = FMath::Max(ClampedMidStart + StreamingBandHysteresis, 0.0f);
	const float AdjustedMidEnd = FMath::Max(ClampedMidEnd + StreamingBandHysteresis, AdjustedMidStart);
	const float AdjustedFarEnd = FMath::Max(ClampedFarEnd + StreamingBandHysteresis, AdjustedMidEnd);
	const float SeedStep = 1000.0f / FMath::Max3(NearDensity, MidDensity, FarDensity);
	const float HalfWidth = FieldWidth * 0.5f;
	const float HalfHeight = FieldHeight * 0.5f;
	const float ClampedStreamingJitter = FMath::Clamp(StreamingStepJitter, 0.0f, 1.0f);
	const float ClampedDropoutMin = FMath::Clamp(StreamingDropoutMin, 0.0f, 1.0f);
	const float ClampedDropoutMax = FMath::Clamp(StreamingDropoutMax, 0.0f, 1.0f);
	const float DropoutMean = (ClampedDropoutMin + ClampedDropoutMax) * 0.5f;
	const float ClampedFrameRoll = FMath::Clamp(StreamingFrameRollStrength, 0.0f, 1.0f);
	const float ClampedDistanceWarp = FMath::Max(StreamingDistanceWarp, 0.0f);
	const float ClampedNoiseFrequency = FMath::Max(StreamingNoiseFrequency, 0.0f);
	const float ClampedRadialNoiseAmplitude = FMath::Clamp(StreamingRadialNoiseAmplitude, 0.0f, 1.0f);
	const float ClampedRadialNoiseFrequency = FMath::Max(StreamingRadialNoiseFrequency, 0.0f);
	const float ClampedClusterChance = FMath::Clamp(StreamingClusterChance, 0.0f, 1.0f);
	const int32 ClampedClusterMaxExtra = FMath::Max(StreamingClusterMaxExtra, 0);
	const float ClampedClusterRadius = FMath::Max(StreamingClusterRadius, 0.0f);
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
	const int32 EffectiveNearLimit = FMath::FloorToInt(static_cast<float>(NearLimit) * (1.0f - DropoutMean));
	const int32 EffectiveMidLimit = FMath::FloorToInt(static_cast<float>(MidLimit) * (1.0f - DropoutMean));
	const int32 EffectiveFarLimit = FMath::FloorToInt(static_cast<float>(FarLimit) * (1.0f - DropoutMean));

	if (InstanceLimit == 0 || UpdateLimit == 0)
	{
		AsteroidHISM->ClearInstances();
		AsteroidMidHISM->ClearInstances();
		AsteroidFarHISM->ClearInstances();
		return;
	}

	const float InputKey = FieldSpline->FindInputKeyClosestToWorldLocation(ViewLocation);
	const float CenterDistance = FieldSpline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
	static FVector LastStreamingViewLocation = FVector::ZeroVector;
	const float RawStart = CenterDistance - EffectiveStreamingRadius;
	const float RawEnd = CenterDistance + EffectiveStreamingRadius;
	const bool bClosedLoop = FieldSpline->IsClosedLoop();

	if (StreamingChunkLength > 0.0f)
	{
		LastStreamingViewLocation = ViewLocation;
		const float ChunkLength = FMath::Max(StreamingChunkLength, 500.0f);
		const int32 ChunkCount = FMath::Max(1, FMath::CeilToInt(SplineLength / ChunkLength));
		const float AddRadius = EffectiveStreamingRadius + StreamingBandHysteresis;
		const float RemoveRadius = FMath::Max(EffectiveStreamingRadius - StreamingBandHysteresis, 0.0f);
		const float AddStart = CenterDistance - AddRadius;
		const float AddEnd = CenterDistance + AddRadius;
		const float RemoveStart = CenterDistance - RemoveRadius;
		const float RemoveEnd = CenterDistance + RemoveRadius;
		TSet<int32> DesiredChunks;
		TSet<int32> KeepChunks;

		auto AddChunkRange = [&](float StartDistance, float EndDistance, TSet<int32>& OutSet)
		{
			if (!bClosedLoop)
			{
				StartDistance = FMath::Clamp(StartDistance, 0.0f, SplineLength);
				EndDistance = FMath::Clamp(EndDistance, 0.0f, SplineLength);
				if (EndDistance < StartDistance)
				{
					return;
				}
			}
			const int32 StartChunk = FMath::FloorToInt(StartDistance / ChunkLength);
			const int32 EndChunk = FMath::FloorToInt(EndDistance / ChunkLength);
			for (int32 ChunkIndex = StartChunk; ChunkIndex <= EndChunk; ++ChunkIndex)
			{
				const int32 Wrapped = bClosedLoop ? ((ChunkIndex % ChunkCount) + ChunkCount) % ChunkCount : ChunkIndex;
				OutSet.Add(Wrapped);
			}
		};

		auto AddWrappedRange = [&](float StartDistance, float EndDistance, TSet<int32>& OutSet)
		{
			if (bClosedLoop && (StartDistance < 0.0f || EndDistance > SplineLength))
			{
				if (StartDistance < 0.0f)
				{
					AddChunkRange(0.0f, FMath::Min(EndDistance, SplineLength), OutSet);
					AddChunkRange(SplineLength + StartDistance, SplineLength, OutSet);
				}
				else
				{
					AddChunkRange(StartDistance, SplineLength, OutSet);
					AddChunkRange(0.0f, EndDistance - SplineLength, OutSet);
				}
			}
			else
			{
				AddChunkRange(StartDistance, EndDistance, OutSet);
			}
		};

		AddWrappedRange(AddStart, AddEnd, DesiredChunks);
		AddWrappedRange(RemoveStart, RemoveEnd, KeepChunks);

		for (auto It = ActiveStreamingChunks.CreateIterator(); It; ++It)
		{
			if (!KeepChunks.Contains(It.Key()))
			{
				FStreamingChunk& Chunk = It.Value();
				if (Chunk.Near)
				{
					Chunk.Near->DestroyComponent();
					StreamingChunkComponents.Remove(Chunk.Near);
				}
				if (Chunk.Mid)
				{
					Chunk.Mid->DestroyComponent();
					StreamingChunkComponents.Remove(Chunk.Mid);
				}
				if (Chunk.Far)
				{
					Chunk.Far->DestroyComponent();
					StreamingChunkComponents.Remove(Chunk.Far);
				}
				It.RemoveCurrent();
			}
		}

		int32 NearCount = 0;
		int32 MidCount = 0;
		int32 FarCount = 0;
		for (const TPair<int32, FStreamingChunk>& Entry : ActiveStreamingChunks)
		{
			const FStreamingChunk& Chunk = Entry.Value;
			if (Chunk.Near)
			{
				NearCount += Chunk.Near->GetInstanceCount();
			}
			if (Chunk.Mid)
			{
				MidCount += Chunk.Mid->GetInstanceCount();
			}
			if (Chunk.Far)
			{
				FarCount += Chunk.Far->GetInstanceCount();
			}
		}

		auto GetChunkBand = [&](float DistanceToView, int32 PreviousBand)
		{
			const float Hysteresis = FMath::Max(StreamingBandHysteresis, 0.0f);
			if (PreviousBand == 0)
			{
				return (DistanceToView > ClampedMidStart + Hysteresis) ? 1 : 0;
			}
			if (PreviousBand == 1)
			{
				if (DistanceToView < FMath::Max(ClampedMidStart - Hysteresis, 0.0f))
				{
					return 0;
				}
				return (DistanceToView > ClampedMidEnd + Hysteresis) ? 2 : 1;
			}
			if (PreviousBand == 2)
			{
				return (DistanceToView < FMath::Max(ClampedMidEnd - Hysteresis, 0.0f)) ? 1 : 2;
			}
			if (DistanceToView <= ClampedMidStart)
			{
				return 0;
			}
			if (DistanceToView <= ClampedMidEnd)
			{
				return 1;
			}
			return 2;
		};

		auto CreateChunkComponent = [&](const FString& BaseName, int32 ChunkIndex, UStaticMesh* Mesh, bool bCollision)
		{
			const FName ComponentName(*FString::Printf(TEXT("%s_%d"), *BaseName, ChunkIndex));
			UHierarchicalInstancedStaticMeshComponent* NewComp = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, ComponentName);
			NewComp->SetupAttachment(BodyMesh);
			NewComp->RegisterComponent();
			NewComp->SetStaticMesh(Mesh);
			NewComp->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
			NewComp->SetNotifyRigidBodyCollision(bCollision);
			NewComp->SetVisibility(true, true);
			StreamingChunkComponents.Add(NewComp);
			if (bCollision)
			{
				NewComp->OnComponentHit.AddDynamic(this, &ANavStaticBig::OnAsteroidHit);
			}
			return NewComp;
		};

		for (int32 ChunkIndex : DesiredChunks)
		{
			FStreamingChunk* Chunk = ActiveStreamingChunks.Find(ChunkIndex);
			if (!Chunk)
			{
				FStreamingChunk NewChunk;
				NewChunk.Near = CreateChunkComponent(TEXT("AsteroidChunkNear"), ChunkIndex, AsteroidMeshes[0], bEnableNearCollision);
				NewChunk.Mid = CreateChunkComponent(TEXT("AsteroidChunkMid"), ChunkIndex, MidAsteroidMesh ? MidAsteroidMesh : AsteroidMeshes[0], false);
				NewChunk.Far = CreateChunkComponent(TEXT("AsteroidChunkFar"), ChunkIndex, FarAsteroidMesh ? FarAsteroidMesh : AsteroidMeshes[0], false);
				ActiveStreamingChunks.Add(ChunkIndex, NewChunk);
				Chunk = ActiveStreamingChunks.Find(ChunkIndex);
			}
			if (!Chunk)
			{
				continue;
			}

			const float ChunkStart = static_cast<float>(ChunkIndex) * ChunkLength;
			const float ChunkEnd = FMath::Min(ChunkStart + ChunkLength, SplineLength);
			const float ChunkCenter = FMath::Clamp(ChunkStart + (ChunkLength * 0.5f), 0.0f, SplineLength);
			const FVector ChunkLocation = FieldSpline->GetLocationAtDistanceAlongSpline(ChunkCenter, ESplineCoordinateSpace::World);
			const float ChunkDistanceToView = FVector::Dist(ChunkLocation, ViewLocation);
			const int32 DesiredBand = GetChunkBand(ChunkDistanceToView, Chunk->LastBand);
			const bool bNeedsRebuild = Chunk->LastBand == INDEX_NONE || DesiredBand != Chunk->LastBand;
			if (!bNeedsRebuild)
			{
				continue;
			}
			Chunk->LastBand = DesiredBand;
			if (Chunk->Near)
			{
				NearCount = FMath::Max(NearCount - Chunk->Near->GetInstanceCount(), 0);
				Chunk->Near->ClearInstances();
			}
			if (Chunk->Mid)
			{
				MidCount = FMath::Max(MidCount - Chunk->Mid->GetInstanceCount(), 0);
				Chunk->Mid->ClearInstances();
			}
			if (Chunk->Far)
			{
				FarCount = FMath::Max(FarCount - Chunk->Far->GetInstanceCount(), 0);
				Chunk->Far->ClearInstances();
			}

			const float ChunkLengthLocal = FMath::Max(ChunkEnd - ChunkStart, 1.0f);
			const float EffectiveDensityScale = FMath::Clamp(1.0f - DropoutMean, 0.1f, 1.0f);
			const int32 CandidateCount = FMath::Max(1, FMath::CeilToInt((ChunkLengthLocal / 1000.0f) * NearDensity * EffectiveDensityScale));
			const float SegmentLength = ChunkLengthLocal / static_cast<float>(CandidateCount);

			for (int32 CandidateIndex = 0; CandidateIndex < CandidateCount; ++CandidateIndex)
			{
				if (NearCount >= NearLimit && MidCount >= MidLimit && FarCount >= FarLimit)
				{
					break;
				}
				FRandomStream CandidateStream(Seed + ChunkIndex * 4099 + CandidateIndex * 53);
				const float RandomAlpha = CandidateStream.FRand();
				const float SegmentAlpha = FMath::Lerp(0.5f, RandomAlpha, ClampedStreamingJitter);
				const float BaseDistance = ChunkStart + (static_cast<float>(CandidateIndex) + SegmentAlpha) * SegmentLength;
				const float NoisePhase = BaseDistance * ClampedNoiseFrequency;
				const float NoiseWarp = FMath::Sin(NoisePhase) * ClampedDistanceWarp;
				const float Distance = FMath::Clamp(BaseDistance + NoiseWarp, ChunkStart, ChunkEnd);
				const FVector Location = FieldSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
				const FVector Tangent = FieldSpline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
				const FVector Forward = Tangent.GetSafeNormal();
				const FMatrix Frame = FRotationMatrix::MakeFromX(Forward);
				FVector Right = Frame.GetScaledAxis(EAxis::Y);
				FVector Up = Frame.GetScaledAxis(EAxis::Z);
				if (ClampedFrameRoll > 0.0f)
				{
					const float RollRadians = CandidateStream.FRandRange(0.0f, 2.0f * PI) * ClampedFrameRoll;
					Right = Right.RotateAngleAxis(FMath::RadiansToDegrees(RollRadians), Forward);
					Up = Up.RotateAngleAxis(FMath::RadiansToDegrees(RollRadians), Forward);
				}
				const float DropoutPhase = (static_cast<float>(ChunkIndex) / FMath::Max(ChunkCount, 1)) * 2.0f * PI;
				const float DropoutWave = (FMath::Sin(DropoutPhase) + 1.0f) * 0.5f;
				const float DropoutChance = FMath::Lerp(ClampedDropoutMin, ClampedDropoutMax, DropoutWave);
				if (CandidateStream.FRand() < DropoutChance)
				{
					continue;
				}
				const float RadialPhase = Distance * ClampedRadialNoiseFrequency;
				const float RadialWarp = 1.0f + (FMath::Sin(RadialPhase) * ClampedRadialNoiseAmplitude);
				const float Angle = CandidateStream.FRandRange(0.0f, 2.0f * PI);
				const float Radius = FMath::Sqrt(CandidateStream.FRand()) * RadialWarp;
				const float OffsetRight = FMath::Cos(Angle) * HalfWidth * Radius;
				const float OffsetUp = FMath::Sin(Angle) * HalfHeight * Radius;
				const FVector InstanceLocation = Location + (Right * OffsetRight) + (Up * OffsetUp);
				const float DistanceToView = FVector::Dist(InstanceLocation, ViewLocation);
				UHierarchicalInstancedStaticMeshComponent* TargetHISM = nullptr;
				float SpawnChance = 0.0f;
				if (DistanceToView <= AdjustedMidStart && NearCount < EffectiveNearLimit)
				{
					TargetHISM = Chunk->Near;
					SpawnChance = ClampedNearSpawn;
				}
				else if (DistanceToView <= AdjustedMidEnd && MidCount < EffectiveMidLimit)
				{
					TargetHISM = Chunk->Mid;
					SpawnChance = ClampedMidSpawn;
				}
				else if (DistanceToView <= AdjustedFarEnd && FarCount < EffectiveFarLimit)
				{
					TargetHISM = Chunk->Far;
					SpawnChance = ClampedFarSpawn;
				}

				if (!TargetHISM)
				{
					continue;
				}

				if (CandidateStream.FRand() > SpawnChance)
				{
					continue;
				}
				auto AddInstanceWithOptionalCluster = [&](const FVector& BaseLocation)
				{
					const FRotator InstanceRotation(
						CandidateStream.FRandRange(0.0f, 360.0f),
						CandidateStream.FRandRange(0.0f, 360.0f),
						CandidateStream.FRandRange(0.0f, 360.0f));
					const float InstanceScale = CandidateStream.FRandRange(MinAsteroidScale, MaxAsteroidScale);
					const FTransform InstanceTransform(InstanceRotation, BaseLocation, FVector(InstanceScale));
					TargetHISM->AddInstance(InstanceTransform, true);
					if (TargetHISM == Chunk->Near)
					{
						++NearCount;
					}
					else if (TargetHISM == Chunk->Mid)
					{
						++MidCount;
					}
					else
					{
						++FarCount;
					}

					if (ClampedClusterMaxExtra <= 0 || CandidateStream.FRand() > ClampedClusterChance)
					{
						return;
					}
					const int32 ExtraCount = CandidateStream.RandRange(1, ClampedClusterMaxExtra);
					for (int32 ExtraIndex = 0; ExtraIndex < ExtraCount; ++ExtraIndex)
					{
						if (NearCount >= EffectiveNearLimit && MidCount >= EffectiveMidLimit && FarCount >= EffectiveFarLimit)
						{
							break;
						}
						const float ClusterAngle = CandidateStream.FRandRange(0.0f, 2.0f * PI);
						const float ClusterRadius = CandidateStream.FRandRange(0.0f, ClampedClusterRadius);
						const FVector ClusterOffset = (Right * FMath::Cos(ClusterAngle) + Up * FMath::Sin(ClusterAngle)) * ClusterRadius;
						const FVector ClusterLocation = BaseLocation + ClusterOffset;
						const FRotator ClusterRotation(
							CandidateStream.FRandRange(0.0f, 360.0f),
							CandidateStream.FRandRange(0.0f, 360.0f),
							CandidateStream.FRandRange(0.0f, 360.0f));
						const float ClusterScale = CandidateStream.FRandRange(MinAsteroidScale, MaxAsteroidScale);
						const FTransform ClusterTransform(ClusterRotation, ClusterLocation, FVector(ClusterScale));
						TargetHISM->AddInstance(ClusterTransform, true);
						if (TargetHISM == Chunk->Near)
						{
							++NearCount;
						}
						else if (TargetHISM == Chunk->Mid)
						{
							++MidCount;
						}
						else
						{
							++FarCount;
						}
					}
				};

				AddInstanceWithOptionalCluster(InstanceLocation);
			}
		}

		return;
	}

	UHierarchicalInstancedStaticMeshComponent* NearTarget = bUseAltStreamBuffer ? AsteroidHISMAlt : AsteroidHISM;
	UHierarchicalInstancedStaticMeshComponent* MidTarget = bUseAltStreamBuffer ? AsteroidMidHISMAlt : AsteroidMidHISM;
	UHierarchicalInstancedStaticMeshComponent* FarTarget = bUseAltStreamBuffer ? AsteroidFarHISMAlt : AsteroidFarHISM;
	UHierarchicalInstancedStaticMeshComponent* NearVisible = bUseAltStreamBuffer ? AsteroidHISM : AsteroidHISMAlt;
	UHierarchicalInstancedStaticMeshComponent* MidVisible = bUseAltStreamBuffer ? AsteroidMidHISM : AsteroidMidHISMAlt;
	UHierarchicalInstancedStaticMeshComponent* FarVisible = bUseAltStreamBuffer ? AsteroidFarHISM : AsteroidFarHISMAlt;

	NearTarget->ClearInstances();
	MidTarget->ClearInstances();
	FarTarget->ClearInstances();
	NearTarget->SetStaticMesh(AsteroidMeshes[0]);
	MidTarget->SetStaticMesh(MidAsteroidMesh ? MidAsteroidMesh : AsteroidMeshes[0]);
	FarTarget->SetStaticMesh(FarAsteroidMesh ? FarAsteroidMesh : AsteroidMeshes[0]);
	NearTarget->SetCollisionEnabled(bEnableNearCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	MidTarget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FarTarget->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	int32 InstancesAdded = 0;
	int32 NearAdded = 0;
	int32 MidAdded = 0;
	int32 FarAdded = 0;
	auto SpawnRange = [&](float StartDistance, float EndDistance)
	{
		const float RangeLength = FMath::Max(EndDistance - StartDistance, 1.0f);
		const float EffectiveDensityScale = FMath::Clamp(1.0f - DropoutMean, 0.1f, 1.0f);
		const int32 CandidateCount = FMath::Max(1, FMath::CeilToInt((RangeLength / 1000.0f) * NearDensity * EffectiveDensityScale));
		const float SegmentLength = RangeLength / static_cast<float>(CandidateCount);
		const int32 RangeSeed = Seed + FMath::FloorToInt(StartDistance);
		for (int32 CandidateIndex = 0; CandidateIndex < CandidateCount && InstancesAdded < InstanceLimit && InstancesAdded < UpdateLimit; ++CandidateIndex)
		{
			FRandomStream CandidateStream(RangeSeed + CandidateIndex * 53);
			const float RandomAlpha = CandidateStream.FRand();
			const float SegmentAlpha = FMath::Lerp(0.5f, RandomAlpha, ClampedStreamingJitter);
			const float BaseDistance = StartDistance + (static_cast<float>(CandidateIndex) + SegmentAlpha) * SegmentLength;
			const float NoisePhase = BaseDistance * ClampedNoiseFrequency;
			const float NoiseWarp = FMath::Sin(NoisePhase) * ClampedDistanceWarp;
			const float Distance = FMath::Clamp(BaseDistance + NoiseWarp, StartDistance, EndDistance);
			const FVector Location = FieldSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			const float DistanceToView = FVector::Dist(Location, ViewLocation);
			const float DropoutPhase = (Distance / FMath::Max(SplineLength, 1.0f)) * 2.0f * PI;
			const float DropoutWave = (FMath::Sin(DropoutPhase) + 1.0f) * 0.5f;
			const float DropoutChance = FMath::Lerp(ClampedDropoutMin, ClampedDropoutMax, DropoutWave);
			if (CandidateStream.FRand() < DropoutChance)
			{
				continue;
			}
			float Density = 0.0f;
			UHierarchicalInstancedStaticMeshComponent* TargetHISM = nullptr;
			float SpawnChance = 0.0f;
			if (DistanceToView <= ClampedMidStart)
			{
				if (NearAdded < EffectiveNearLimit)
				{
					Density = NearDensity;
					TargetHISM = NearTarget;
					SpawnChance = ClampedNearSpawn;
				}
			}
			else if (DistanceToView <= ClampedMidEnd)
			{
				if (MidAdded < EffectiveMidLimit)
				{
					Density = MidDensity;
					TargetHISM = MidTarget;
					SpawnChance = ClampedMidSpawn;
				}
			}
			else if (DistanceToView <= ClampedFarEnd)
			{
				if (FarAdded < EffectiveFarLimit)
				{
					Density = FarDensity;
					TargetHISM = FarTarget;
					SpawnChance = ClampedFarSpawn;
				}
			}

			const float Step = (Density > 0.0f) ? (1000.0f / Density) : (1000.0f / NearDensity);
			const FVector Tangent = FieldSpline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
			const FVector Forward = Tangent.GetSafeNormal();
			const FMatrix Frame = FRotationMatrix::MakeFromX(Forward);
			FVector Right = Frame.GetScaledAxis(EAxis::Y);
			FVector Up = Frame.GetScaledAxis(EAxis::Z);
			if (ClampedFrameRoll > 0.0f)
			{
				const float RollRadians = CandidateStream.FRandRange(0.0f, 2.0f * PI) * ClampedFrameRoll;
				Right = Right.RotateAngleAxis(FMath::RadiansToDegrees(RollRadians), Forward);
				Up = Up.RotateAngleAxis(FMath::RadiansToDegrees(RollRadians), Forward);
			}

			if (Density > 0.0f && TargetHISM)
			{
				if (CandidateStream.FRand() > SpawnChance)
				{
					continue;
				}
				const float RadialPhase = Distance * ClampedRadialNoiseFrequency;
				const float RadialWarp = 1.0f + (FMath::Sin(RadialPhase) * ClampedRadialNoiseAmplitude);
				const float Angle = CandidateStream.FRandRange(0.0f, 2.0f * PI);
				const float Radius = FMath::Sqrt(CandidateStream.FRand()) * RadialWarp;
				const float OffsetRight = FMath::Cos(Angle) * HalfWidth * Radius;
				const float OffsetUp = FMath::Sin(Angle) * HalfHeight * Radius;
				const FVector InstanceLocation = Location + (Right * OffsetRight) + (Up * OffsetUp);

				auto AddInstanceWithOptionalCluster = [&](const FVector& BaseLocation)
				{
					const FRotator InstanceRotation(
						CandidateStream.FRandRange(0.0f, 360.0f),
						CandidateStream.FRandRange(0.0f, 360.0f),
						CandidateStream.FRandRange(0.0f, 360.0f));
					const float InstanceScale = CandidateStream.FRandRange(MinAsteroidScale, MaxAsteroidScale);
					const FTransform InstanceTransform(InstanceRotation, BaseLocation, FVector(InstanceScale));
					TargetHISM->AddInstance(InstanceTransform, true);
					++InstancesAdded;
					if (TargetHISM == NearTarget)
					{
						++NearAdded;
					}
					else if (TargetHISM == MidTarget)
					{
						++MidAdded;
					}
					else if (TargetHISM == FarTarget)
					{
						++FarAdded;
					}

					if (ClampedClusterMaxExtra <= 0 || CandidateStream.FRand() > ClampedClusterChance)
					{
						return;
					}
					const int32 ExtraCount = CandidateStream.RandRange(1, ClampedClusterMaxExtra);
					for (int32 ExtraIndex = 0; ExtraIndex < ExtraCount; ++ExtraIndex)
					{
						if (NearAdded >= EffectiveNearLimit && MidAdded >= EffectiveMidLimit && FarAdded >= EffectiveFarLimit)
						{
							break;
						}
						const float ClusterAngle = CandidateStream.FRandRange(0.0f, 2.0f * PI);
						const float ClusterRadius = CandidateStream.FRandRange(0.0f, ClampedClusterRadius);
						const FVector ClusterOffset = (Right * FMath::Cos(ClusterAngle) + Up * FMath::Sin(ClusterAngle)) * ClusterRadius;
						const FVector ClusterLocation = BaseLocation + ClusterOffset;
						const FRotator ClusterRotation(
							CandidateStream.FRandRange(0.0f, 360.0f),
							CandidateStream.FRandRange(0.0f, 360.0f),
							CandidateStream.FRandRange(0.0f, 360.0f));
						const float ClusterScale = CandidateStream.FRandRange(MinAsteroidScale, MaxAsteroidScale);
						const FTransform ClusterTransform(ClusterRotation, ClusterLocation, FVector(ClusterScale));
						TargetHISM->AddInstance(ClusterTransform, true);
						++InstancesAdded;
						if (TargetHISM == NearTarget)
						{
							++NearAdded;
						}
						else if (TargetHISM == MidTarget)
						{
							++MidAdded;
						}
						else if (TargetHISM == FarTarget)
						{
							++FarAdded;
						}
					}
				};

				AddInstanceWithOptionalCluster(InstanceLocation);
			}
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

	NearTarget->SetVisibility(true, true);
	MidTarget->SetVisibility(true, true);
	FarTarget->SetVisibility(true, true);
	NearVisible->SetVisibility(false, true);
	MidVisible->SetVisibility(false, true);
	FarVisible->SetVisibility(false, true);
	NearVisible->ClearInstances();
	MidVisible->ClearInstances();
	FarVisible->ClearInstances();
	bUseAltStreamBuffer = !bUseAltStreamBuffer;
}

void ANavStaticBig::OnAsteroidHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bSpawnDynamicOnHit || ActiveDynamicAsteroids >= MaxDynamicAsteroids || !DynamicAsteroidClass)
	{
		return;
	}

	if (!AsteroidHISM || !AsteroidHISMAlt)
	{
		return;
	}

	if (HitComponent != AsteroidHISM && HitComponent != AsteroidHISMAlt)
	{
		return;
	}

	const int32 InstanceIndex = Hit.Item;
	if (InstanceIndex < 0)
	{
		return;
	}

	UHierarchicalInstancedStaticMeshComponent* HitHISM = Cast<UHierarchicalInstancedStaticMeshComponent>(HitComponent);
	if (!HitHISM)
	{
		return;
	}

	SpawnDynamicAsteroidFromInstance(HitHISM, InstanceIndex, NormalImpulse, true);
}

bool ANavStaticBig::ReplaceHISMInstanceWithActor(UHierarchicalInstancedStaticMeshComponent* SourceHISM, int32 InstanceIndex)
{
	const FVector EmptyImpulse = FVector::ZeroVector;
	return SpawnDynamicAsteroidFromInstance(SourceHISM, InstanceIndex, EmptyImpulse, false);
}

bool ANavStaticBig::SpawnDynamicAsteroidFromInstance(UHierarchicalInstancedStaticMeshComponent* SourceHISM, int32 InstanceIndex,
	const FVector& NormalImpulse, bool bApplyImpulse)
{
	if (!SourceHISM || InstanceIndex < 0 || !DynamicAsteroidClass)
	{
		return false;
	}

	if (ActiveDynamicAsteroids >= MaxDynamicAsteroids)
	{
		return false;
	}

	FTransform InstanceTransform;
	if (!SourceHISM->GetInstanceTransform(InstanceIndex, InstanceTransform, true))
	{
		return false;
	}

	UStaticMesh* SourceMesh = SourceHISM->GetStaticMesh();
	if (!SourceMesh)
	{
		return false;
	}

	SourceHISM->RemoveInstance(InstanceIndex);
	const FVector SpawnLocation = InstanceTransform.GetLocation();
	const FRotator SpawnRotation = InstanceTransform.Rotator();
	const FVector SpawnScale = InstanceTransform.GetScale3D();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* SpawnedAsteroid = GetWorld()->SpawnActor<AActor>(DynamicAsteroidClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!SpawnedAsteroid)
	{
		return false;
	}

	SpawnedAsteroid->SetActorScale3D(SpawnScale);
	if (UStaticMeshComponent* MeshComponent = SpawnedAsteroid->FindComponentByClass<UStaticMeshComponent>())
	{
		MeshComponent->SetStaticMesh(SourceMesh);
	}
	SpawnedAsteroid->OnDestroyed.AddDynamic(this, &ANavStaticBig::OnDynamicAsteroidDestroyed);
	++ActiveDynamicAsteroids;

	if (bApplyImpulse)
	{
		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(SpawnedAsteroid->GetRootComponent()))
		{
			RootPrimitive->SetSimulatePhysics(true);
			RootPrimitive->AddImpulseAtLocation(NormalImpulse.GetSafeNormal() * HitImpulseStrength, SpawnLocation);
		}
	}

	return true;
}

void ANavStaticBig::OnDynamicAsteroidDestroyed(AActor* DestroyedActor)
{
	ActiveDynamicAsteroids = FMath::Max(ActiveDynamicAsteroids - 1, 0);
}

FVector ANavStaticBig::GetStreamingViewLocation() const
{
	if (!GetWorld())
	{
		return GetActorLocation();
	}

	if (const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		if (const APawn* Pawn = PlayerController->GetPawn())
		{
			return Pawn->GetActorLocation();
		}
		if (const AActor* ViewTarget = PlayerController->GetViewTarget())
		{
			return ViewTarget->GetActorLocation();
		}
	}

	return GetActorLocation();
}

