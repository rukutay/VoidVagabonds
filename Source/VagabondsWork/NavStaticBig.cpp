// Fill out your copyright notice in the Description page of Project Settings.


#include "NavStaticBig.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSubsystem.h"
#include "MarkerComponent.h"

static void BeginHISMBatch(UHierarchicalInstancedStaticMeshComponent* HISM)
{
	if (!HISM)
	{
		return;
	}

	HISM->bAutoRebuildTreeOnInstanceChanges = false;
}

static void EndHISMBatch(UHierarchicalInstancedStaticMeshComponent* HISM)
{
	if (!HISM)
	{
		return;
	}

	HISM->BuildTreeIfOutdated(false, true);
	HISM->MarkRenderStateDirty();
}

// Sets default values
ANavStaticBig::ANavStaticBig()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	SetRootComponent(BodyMesh);

	MarkerComponent = CreateDefaultSubobject<UMarkerComponent>(TEXT("MarkerComponent"));
	MarkerComponent->MarkerType = EMarkerType::Planet;

	SignatureSphere = CreateDefaultSubobject<USphereComponent>(TEXT("SignatureSphere"));
	SignatureSphere->SetupAttachment(BodyMesh);

	FieldSpline = CreateDefaultSubobject<USplineComponent>(TEXT("FieldSpline"));
	FieldSpline->SetupAttachment(BodyMesh);

	AsteroidHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("AsteroidHISM"));
	AsteroidHISM->SetupAttachment(BodyMesh);
	AsteroidHISM->bAutoRebuildTreeOnInstanceChanges = false;

	SignatureSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidHISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AsteroidHISM->SetNotifyRigidBodyCollision(true);

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

	if (bEnableStreaming && StreamingUpdateInterval > 0.0f)
	{
		UpdateAsteroidStreaming();
		GetWorldTimerManager().SetTimer(StreamingTimerHandle, this, &ANavStaticBig::UpdateAsteroidStreaming, StreamingUpdateInterval, true);
	}

	if (bEnableNearActorSwap && NearActorSwapInterval > 0.0f && !bEnableStreaming)
	{
		UpdateNearAsteroidActorSwap();
		GetWorldTimerManager().SetTimer(NearSwapTimerHandle, this, &ANavStaticBig::UpdateNearAsteroidActorSwap, NearActorSwapInterval, true);
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
		else if (SignatureSphere)
		{
			EffectiveRadius = SignatureSphere->GetScaledSphereRadius() * 1.5f;
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

UStaticMesh* ANavStaticBig::GetRandomAsteroidMesh(FRandomStream& RandomStream) const
{
	if (AsteroidMeshes.Num() == 0)
	{
		return nullptr;
	}

	for (int32 Attempt = 0; Attempt < AsteroidMeshes.Num(); ++Attempt)
	{
		const int32 Index = RandomStream.RandRange(0, AsteroidMeshes.Num() - 1);
		if (UStaticMesh* Mesh = AsteroidMeshes[Index])
		{
			return Mesh;
		}
	}

	for (UStaticMesh* Mesh : AsteroidMeshes)
	{
		if (Mesh)
		{
			return Mesh;
		}
	}

	return nullptr;
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

	FRandomStream MeshStream(Seed + 19);
	UStaticMesh* SelectedMesh = GetRandomAsteroidMesh(MeshStream);
	if (!SelectedMesh)
	{
		return;
	}

	const float ClampedJitterMin = FMath::Max(StepJitterMin, 0.05f);
	const float ClampedJitterMax = FMath::Max(StepJitterMax, ClampedJitterMin);

	AsteroidHISM->ClearInstances();
	AsteroidHISM->SetStaticMesh(SelectedMesh);
	AsteroidHISM->ForcedLodModel = 0;

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

	FRandomStream MeshStream(Seed + 19);
	UStaticMesh* BaseMesh = GetRandomAsteroidMesh(MeshStream);
	if (!BaseMesh)
	{
		return;
	}
	const float ClampedJitterMin = FMath::Max(StepJitterMin, 0.05f);
	const float ClampedJitterMax = FMath::Max(StepJitterMax, ClampedJitterMin);
	const float ClampedNearSpawn = FMath::Clamp(NearSpawnProbability, 0.0f, 1.0f);
	const FVector ViewLocation = GetStreamingViewLocation();
	const float SplineLength = FieldSpline->GetSplineLength();
	const float EffectiveStreamingRadius = StreamingRadius > 0.0f ? StreamingRadius : SplineLength;
	const float NearDensity = FMath::Max(DensityPer1000uu, 0.001f);
	const float AdjustedFarEnd = BIG_NUMBER;
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
	uint32 StreamingConfigHash = 0;
	int32 InstanceLimit = FMath::Max(MaxAsteroidInstances, 0);
	int32 UpdateLimit = InstanceLimit;
	int32 NearLimit = FMath::Max(MaxNearInstances, 0);

#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		InstanceLimit = FMath::Min(InstanceLimit, FMath::Max(PreviewMaxInstancesEditor, 0));
		UpdateLimit = InstanceLimit;
		NearLimit = FMath::Min(NearLimit, InstanceLimit);
	}
#endif

	if (MaxInstancesPerUpdate > 0)
	{
		UpdateLimit = FMath::Min(UpdateLimit, MaxInstancesPerUpdate);
	}
	NearLimit = FMath::Min(NearLimit, InstanceLimit);
	const int32 EffectiveNearLimit = FMath::FloorToInt(static_cast<float>(NearLimit) * (1.0f - DropoutMean));

	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(Seed));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(FieldWidth));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(FieldHeight));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(EffectiveNearLimit));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(NearDensity));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedNearSpawn));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedStreamingJitter));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedDropoutMin));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedDropoutMax));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedFrameRoll));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedDistanceWarp));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedNoiseFrequency));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedRadialNoiseAmplitude));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedRadialNoiseFrequency));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedClusterChance));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedClusterMaxExtra));
	StreamingConfigHash = HashCombine(StreamingConfigHash, GetTypeHash(ClampedClusterRadius));
	const bool bStreamingConfigChanged = StreamingConfigHash != LastStreamingConfigHash;
	LastStreamingConfigHash = StreamingConfigHash;

	if (InstanceLimit == 0 || UpdateLimit == 0)
	{
		BeginHISMBatch(AsteroidHISM);
		AsteroidHISM->ClearInstances();
		EndHISMBatch(AsteroidHISM);
		return;
	}

	const float InputKey = FieldSpline->FindInputKeyClosestToWorldLocation(ViewLocation);
	const float CenterDistance = FieldSpline->GetDistanceAlongSplineAtSplineInputKey(InputKey);
	static FVector LastStreamingViewLocation = FVector::ZeroVector;
	const float RawStart = CenterDistance - EffectiveStreamingRadius;
	const float RawEnd = CenterDistance + EffectiveStreamingRadius;
	const bool bClosedLoop = FieldSpline->IsClosedLoop();
	const float EffectiveChunkLength = (StreamingChunkLength > 0.0f) ? StreamingChunkLength : 5000.0f;

	if (EffectiveChunkLength > 0.0f)
	{
		LastStreamingViewLocation = ViewLocation;
		const float ChunkLength = FMath::Max(EffectiveChunkLength, 500.0f);
		const int32 ChunkCount = FMath::Max(1, FMath::CeilToInt(SplineLength / ChunkLength));
		const float ChunkBandHysteresis = FMath::Max(StreamingBandHysteresis, 0.0f) * 2.0f;
		const float AddRadius = EffectiveStreamingRadius + ChunkBandHysteresis;
		const float RemoveRadius = FMath::Max(EffectiveStreamingRadius - ChunkBandHysteresis, 0.0f);
		const float AddStart = CenterDistance - AddRadius;
		const float AddEnd = CenterDistance + AddRadius;
		const float RemoveStart = CenterDistance - RemoveRadius;
		const float RemoveEnd = CenterDistance + RemoveRadius;
		TSet<int32> DesiredChunks;
		TSet<int32> KeepChunks;
		const bool bSpawnNearActors = bEnableNearActorSwap && DynamicAsteroidClass;
		auto CountNearSwapActorsForComponent = [&](UHierarchicalInstancedStaticMeshComponent* Component)
		{
			int32 Count = 0;
			if (!Component)
			{
				return Count;
			}
			for (const TPair<TWeakObjectPtr<AActor>, FNearSwapEntry>& Entry : ActiveNearSwapActors)
			{
				if (Entry.Value.SourceHISM.IsValid() && Entry.Value.SourceHISM.Get() == Component)
				{
					++Count;
				}
			}
			return Count;
		};

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

		TArray<int32> OrderedChunks = DesiredChunks.Array();
		OrderedChunks.Sort();
		const int32 DesiredChunkCount = FMath::Max(OrderedChunks.Num(), 1);
		const int32 NearBaseBudget = (EffectiveNearLimit > 0) ? (EffectiveNearLimit / DesiredChunkCount) : 0;
		const int32 NearRemainder = (EffectiveNearLimit > 0) ? (EffectiveNearLimit % DesiredChunkCount) : 0;

		for (auto It = ActiveStreamingChunks.CreateIterator(); It; ++It)
		{
			if (!KeepChunks.Contains(It.Key()))
			{
				FStreamingChunk& Chunk = It.Value();
				if (Chunk.Near)
				{
					CleanupNearSwapForComponent(Chunk.Near);
					Chunk.Near->DestroyComponent();
					StreamingChunkComponents.Remove(Chunk.Near);
				}
				It.RemoveCurrent();
			}
		}

		int32 NearCount = 0;
		for (const TPair<int32, FStreamingChunk>& Entry : ActiveStreamingChunks)
		{
			const FStreamingChunk& Chunk = Entry.Value;
			if (Chunk.Near)
			{
				NearCount += Chunk.Near->GetInstanceCount();
			}
		}
		if (bSpawnNearActors)
		{
			for (const TPair<TWeakObjectPtr<AActor>, FNearSwapEntry>& Entry : ActiveNearSwapActors)
			{
				if (Entry.Value.SourceHISM.IsValid()
					&& StreamingChunkComponents.Contains(Entry.Value.SourceHISM.Get()))
				{
					++NearCount;
				}
			}
		}

		auto CreateChunkComponent = [&](const FString& BaseName, int32 ChunkIndex, UStaticMesh* Mesh, bool bCollision, int32 ForcedLod)
		{
			const FName ComponentName(*FString::Printf(TEXT("%s_%d"), *BaseName, ChunkIndex));
			UHierarchicalInstancedStaticMeshComponent* NewComp = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, ComponentName);
			NewComp->SetupAttachment(BodyMesh);
			NewComp->RegisterComponent();
			NewComp->SetStaticMesh(Mesh);
			NewComp->bAutoRebuildTreeOnInstanceChanges = false;
			NewComp->ForcedLodModel = ForcedLod;
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

		for (int32 OrderedIndex = 0; OrderedIndex < OrderedChunks.Num(); ++OrderedIndex)
		{
			const int32 ChunkIndex = OrderedChunks[OrderedIndex];
			const int32 NearChunkBudget = NearBaseBudget + ((OrderedIndex < NearRemainder) ? 1 : 0);
			FStreamingChunk* Chunk = ActiveStreamingChunks.Find(ChunkIndex);
			if (!Chunk)
			{
				FRandomStream ChunkMeshStream(Seed + ChunkIndex * 4099 + 17);
				UStaticMesh* ChunkMesh = GetRandomAsteroidMesh(ChunkMeshStream);
				if (!ChunkMesh)
				{
					ChunkMesh = BaseMesh;
				}
				FStreamingChunk NewChunk;
				NewChunk.Near = CreateChunkComponent(TEXT("AsteroidChunkNear"), ChunkIndex, ChunkMesh, bEnableNearCollision, 0);
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
			const int32 DesiredBand = 0;
			const bool bNeedsRebuild = bStreamingConfigChanged || Chunk->LastBand == INDEX_NONE;
			if (!bNeedsRebuild)
			{
				continue;
			}
			Chunk->LastBand = DesiredBand;
			BeginHISMBatch(Chunk->Near);
			if (Chunk->Near)
			{
				const int32 ExistingNearInstances = Chunk->Near->GetInstanceCount();
				const int32 NearActorsToRemove = bSpawnNearActors
					? CountNearSwapActorsForComponent(Chunk->Near)
					: 0;
				CleanupNearSwapForComponent(Chunk->Near);
				NearCount = FMath::Max(NearCount - (ExistingNearInstances + NearActorsToRemove), 0);
				Chunk->Near->ClearInstances();
			}

			const float ChunkLengthLocal = FMath::Max(ChunkEnd - ChunkStart, 1.0f);
			const float EffectiveDensityScale = FMath::Clamp(1.0f - DropoutMean, 0.1f, 1.0f);
			const int32 CandidateCount = FMath::Max(1, FMath::CeilToInt((ChunkLengthLocal / 1000.0f) * NearDensity * EffectiveDensityScale));
			const float SegmentLength = ChunkLengthLocal / static_cast<float>(CandidateCount);

			int32 NearChunkCount = 0;
			for (int32 CandidateIndex = 0; CandidateIndex < CandidateCount; ++CandidateIndex)
			{
				if (NearCount >= EffectiveNearLimit)
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
				if (DistanceToView <= AdjustedFarEnd && NearCount < EffectiveNearLimit && NearChunkCount < NearChunkBudget)
				{
					TargetHISM = Chunk->Near;
					SpawnChance = ClampedNearSpawn;
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
					if (TargetHISM == Chunk->Near && NearChunkCount >= NearChunkBudget)
					{
						return;
					}
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
						++NearChunkCount;
					}

					if (ClampedClusterMaxExtra <= 0 || CandidateStream.FRand() > ClampedClusterChance)
					{
						return;
					}
					const int32 ExtraCount = CandidateStream.RandRange(1, ClampedClusterMaxExtra);
					for (int32 ExtraIndex = 0; ExtraIndex < ExtraCount; ++ExtraIndex)
					{
						if (NearCount >= EffectiveNearLimit || NearChunkCount >= NearChunkBudget)
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
							++NearChunkCount;
						}
					}
				};

				AddInstanceWithOptionalCluster(InstanceLocation);
			}
			EndHISMBatch(Chunk->Near);
		}

		if (bEnableNearActorSwap)
		{
			UpdateNearAsteroidActorSwap();
		}

		return;
	}

}

void ANavStaticBig::OnAsteroidHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bSpawnDynamicOnHit || ActiveDynamicAsteroids >= MaxDynamicAsteroids || !DynamicAsteroidClass)
	{
		return;
	}

	if (!AsteroidHISM)
	{
		return;
	}

	if (HitComponent != AsteroidHISM)
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

void ANavStaticBig::UpdateNearAsteroidActorSwap()
{
	if (!bEnableNearActorSwap || !DynamicAsteroidClass)
	{
		ActiveNearSwapActors.Reset();
		if (UWorld* World = GetWorld())
		{
		if (UNavigationSubsystem* NavigationSubsystem = World->GetGameInstance()->GetSubsystem<UNavigationSubsystem>())
			{
			NavigationSubsystem->SetRuntimeNavObstacleActors(TArray<AActor*>());
			}
		}
		return;
	}
	const FVector ViewLocation = GetStreamingViewLocation();
	const float EnterRadius = FMath::Max(NearActorSwapEnterRadius, 0.0f);
	const float ExitRadius = FMath::Max(NearActorSwapExitRadius, EnterRadius);
	if (EnterRadius <= 0.0f)
	{
		return;
	}
	const float EnterRadiusSq = EnterRadius * EnterRadius;
	const float ExitRadiusSq = ExitRadius * ExitRadius;

	TArray<UHierarchicalInstancedStaticMeshComponent*> Candidates;
	Candidates.Reserve(16);

	if (bEnableStreaming)
	{
		for (const TPair<int32, FStreamingChunk>& Entry : ActiveStreamingChunks)
		{
			if (Entry.Value.Near && Entry.Value.Near->GetStaticMesh())
			{
				Candidates.Add(Entry.Value.Near);
			}
		}
	}
	else
	{
		if (AsteroidHISM)
		{
			Candidates.Add(AsteroidHISM);
		}
	}

	NearSwapHISMComponents.Reset();
	for (UHierarchicalInstancedStaticMeshComponent* Component : Candidates)
	{
		NearSwapHISMComponents.Add(Component);
	}

	for (UHierarchicalInstancedStaticMeshComponent* Component : Candidates)
	{
		if (!Component)
		{
			continue;
		}

		const int32 InstanceCount = Component->GetInstanceCount();
		for (int32 InstanceIndex = InstanceCount - 1; InstanceIndex >= 0; --InstanceIndex)
		{
			FTransform InstanceTransform;
			if (!Component->GetInstanceTransform(InstanceIndex, InstanceTransform, true))
			{
				continue;
			}
			const FVector InstanceLocation = InstanceTransform.GetLocation();
			const float DistSq = FVector::DistSquared(ViewLocation, InstanceLocation);
			if (DistSq <= EnterRadiusSq)
			{
				SpawnNearSwapAsteroidFromInstance(Component, InstanceIndex);
			}
		}
	}

	TArray<TWeakObjectPtr<AActor>> ActorsToRestore;
	ActorsToRestore.Reserve(ActiveNearSwapActors.Num());
	for (const TPair<TWeakObjectPtr<AActor>, FNearSwapEntry>& Entry : ActiveNearSwapActors)
	{
		AActor* Actor = Entry.Key.Get();
		if (!Actor)
		{
			ActorsToRestore.Add(Entry.Key);
			continue;
		}
		const FNearSwapEntry& SwapEntry = Entry.Value;
		if (SwapEntry.bBecameDynamic)
		{
			continue;
		}
		if (!SwapEntry.SourceHISM.IsValid())
		{
			ActorsToRestore.Add(Entry.Key);
			continue;
		}
		const float DistSq = FVector::DistSquared(ViewLocation, Actor->GetActorLocation());
		if (DistSq > ExitRadiusSq)
		{
			ActorsToRestore.Add(Entry.Key);
		}
	}

	for (const TWeakObjectPtr<AActor>& ActorPtr : ActorsToRestore)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			RestoreNearSwapActor(Actor, true);
		}
		else
		{
			ActiveNearSwapActors.Remove(ActorPtr);
		}
	}

	TArray<AActor*> SleepingActors;
	SleepingActors.Reserve(ActiveNearSwapActors.Num());
	for (const TPair<TWeakObjectPtr<AActor>, FNearSwapEntry>& Entry : ActiveNearSwapActors)
	{
		AActor* Actor = Entry.Key.Get();
		if (!Actor)
		{
			continue;
		}
		if (Entry.Value.bBecameDynamic)
		{
			continue;
		}
		UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
		const bool bAwake = RootPrimitive && RootPrimitive->IsSimulatingPhysics() && RootPrimitive->IsAnyRigidBodyAwake();
		if (!bAwake)
		{
			SleepingActors.Add(Actor);
		}
	}
	if (UWorld* World = GetWorld())
	{
		if (UNavigationSubsystem* NavigationSubsystem = World->GetGameInstance()->GetSubsystem<UNavigationSubsystem>())
		{
			NavigationSubsystem->SetRuntimeNavObstacleActors(SleepingActors);
		}
	}
}

void ANavStaticBig::CleanupNearSwapForComponent(UHierarchicalInstancedStaticMeshComponent* Component)
{
	if (!Component)
	{
		return;
	}

	const FVector ViewLocation = GetStreamingViewLocation();
	const float EnterRadius = FMath::Max(NearActorSwapEnterRadius, 0.0f);
	const float ExitRadius = FMath::Max(NearActorSwapExitRadius, EnterRadius);
	const float ExitRadiusSq = ExitRadius * ExitRadius;

	TArray<TWeakObjectPtr<AActor>> ActorsToRestore;
	for (TPair<TWeakObjectPtr<AActor>, FNearSwapEntry>& Entry : ActiveNearSwapActors)
	{
		FNearSwapEntry& SwapEntry = Entry.Value;
		if (SwapEntry.SourceHISM != Component)
		{
			continue;
		}
		AActor* Actor = Entry.Key.Get();
		if (!Actor)
		{
			ActorsToRestore.Add(Entry.Key);
			continue;
		}
		if (SwapEntry.bBecameDynamic)
		{
			continue;
		}
		if (ExitRadius > 0.0f)
		{
			const float DistSq = FVector::DistSquared(ViewLocation, Actor->GetActorLocation());
			if (DistSq <= ExitRadiusSq)
			{
				SwapEntry.bBecameDynamic = true;
				SwapEntry.SourceHISM = nullptr;
				continue;
			}
		}
		ActorsToRestore.Add(Entry.Key);
	}

	for (const TWeakObjectPtr<AActor>& ActorPtr : ActorsToRestore)
	{
		if (AActor* Actor = ActorPtr.Get())
		{
			RestoreNearSwapActor(Actor, true);
		}
		else
		{
			ActiveNearSwapActors.Remove(ActorPtr);
		}
	}
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
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
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

bool ANavStaticBig::SpawnNearSwapAsteroidFromInstance(UHierarchicalInstancedStaticMeshComponent* SourceHISM, int32 InstanceIndex)
{
	if (!SourceHISM || InstanceIndex < 0 || !DynamicAsteroidClass)
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

	if (SourceHISM->RemoveInstance(InstanceIndex) == false)
	{
		return false;
	}

	const FVector SpawnLocation = InstanceTransform.GetLocation();
	const FRotator SpawnRotation = InstanceTransform.Rotator();
	const FVector SpawnScale = InstanceTransform.GetScale3D();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* SpawnedAsteroid = GetWorld()->SpawnActor<AActor>(DynamicAsteroidClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (!SpawnedAsteroid)
	{
		SourceHISM->AddInstance(InstanceTransform, true);
		return false;
	}

	SpawnedAsteroid->SetActorScale3D(SpawnScale);
	if (UStaticMeshComponent* MeshComponent = SpawnedAsteroid->FindComponentByClass<UStaticMeshComponent>())
	{
		MeshComponent->SetStaticMesh(SourceMesh);
		MeshComponent->OnComponentWake.AddDynamic(this, &ANavStaticBig::OnNearSwapAsteroidWake);
	}
	SpawnedAsteroid->OnDestroyed.AddDynamic(this, &ANavStaticBig::OnNearSwapAsteroidDestroyed);

	FNearSwapEntry Entry;
	Entry.SourceHISM = SourceHISM;
	Entry.InstanceTransform = InstanceTransform;
	ActiveNearSwapActors.Add(SpawnedAsteroid, Entry);

#if !UE_BUILD_SHIPPING
	if (bDebugNearActorSwap)
	{
		DrawDebugSphere(GetWorld(), SpawnLocation, 180.0f, 10, FColor::Purple, false, NearActorSwapInterval, 0, 1.2f);
	}
#endif

	return true;
}

bool ANavStaticBig::RestoreNearSwapActor(AActor* SwappedActor, bool bDestroyActor)
{
	if (bRestoringNearSwapActor || !SwappedActor)
	{
		return false;
	}

	const FNearSwapEntry* Entry = ActiveNearSwapActors.Find(SwappedActor);
	if (!Entry)
	{
		return false;
	}

	UHierarchicalInstancedStaticMeshComponent* SourceHISM = Entry->SourceHISM.Get();
	if (SourceHISM)
	{
		SourceHISM->AddInstance(Entry->InstanceTransform, true);
	}

	ActiveNearSwapActors.Remove(SwappedActor);

	if (bDestroyActor)
	{
		bRestoringNearSwapActor = true;
		SwappedActor->Destroy();
		bRestoringNearSwapActor = false;
	}

	return true;
}

void ANavStaticBig::OnDynamicAsteroidDestroyed(AActor* DestroyedActor)
{
	ActiveDynamicAsteroids = FMath::Max(ActiveDynamicAsteroids - 1, 0);
}

void ANavStaticBig::OnNearSwapAsteroidDestroyed(AActor* DestroyedActor)
{
	if (bRestoringNearSwapActor || !DestroyedActor)
	{
		return;
	}

	RestoreNearSwapActor(DestroyedActor, false);
}

void ANavStaticBig::OnNearSwapAsteroidWake(UPrimitiveComponent* WokeComponent, FName BoneName)
{
	if (!WokeComponent)
	{
		return;
	}

	AActor* Owner = WokeComponent->GetOwner();
	if (!Owner)
	{
		return;
	}

	FNearSwapEntry* Entry = ActiveNearSwapActors.Find(Owner);
	if (!Entry)
	{
		return;
	}

	Entry->bBecameDynamic = true;
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