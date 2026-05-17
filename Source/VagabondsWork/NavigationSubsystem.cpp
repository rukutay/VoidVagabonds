#include "NavigationSubsystem.h"

#include "Algo/Reverse.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Math/NumericLimits.h"
#include "Misc/CoreDelegates.h"
#include "NavStaticBig.h"

namespace
{
	bool SegmentIntersectsSphere(const FVector& A, const FVector& B, const FVector& C, float R)
	{
		const FVector AB = B - A;
		const float ABSizeSquared = AB.SizeSquared();
		if (ABSizeSquared <= KINDA_SMALL_NUMBER)
		{
			return (C - A).SizeSquared() <= (R * R);
		}

		const float T = FMath::Clamp(FVector::DotProduct(C - A, AB) / ABSizeSquared, 0.0f, 1.0f);
		const FVector Closest = A + (AB * T);
		return (C - Closest).SizeSquared() <= (R * R);
	}

	AActor* FindNavStaticBigOwner(AActor* Actor)
	{
		for (AActor* Current = Actor; Current; Current = Current->GetAttachParentActor())
		{
			if (Current->IsA<ANavStaticBig>())
			{
				return Current;
			}
		}

		return nullptr;
	}
}

void UNavigationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	TryInitializeForWorld(GetWorld());

	if (!PostLoadMapHandle.IsValid())
	{
		PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
			this,
			&UNavigationSubsystem::HandlePostLoadMap);
	}
}

void UNavigationSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MovingStaticRefreshTimer);
	}

	Super::Deinitialize();
}

void UNavigationSubsystem::HandlePostLoadMap(UWorld* LoadedWorld)
{
	TryInitializeForWorld(LoadedWorld);
}

void UNavigationSubsystem::TryInitializeForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	InitializeNavObstacles();

	World->GetTimerManager().ClearTimer(MovingStaticRefreshTimer);
	World->GetTimerManager().SetTimer(
		MovingStaticRefreshTimer,
		this,
		&UNavigationSubsystem::RefreshMovingStaticObstacles,
		StaticObstacleRefreshInterval,
		true);
}

void UNavigationSubsystem::EnsureNavObstaclesInitialized() const
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	if (bInitializingStaticNavObstacles)
	{
		return;
	}

	if (!bStaticNavObstaclesInitialized || CachedNavWorld.Get() != World || StaticNavObstacles.Num() == 0)
	{
		UNavigationSubsystem* MutableThis = const_cast<UNavigationSubsystem*>(this);
		MutableThis->InitializeNavObstacles();
		World->GetTimerManager().ClearTimer(MutableThis->MovingStaticRefreshTimer);
		World->GetTimerManager().SetTimer(
			MutableThis->MovingStaticRefreshTimer,
			MutableThis,
			&UNavigationSubsystem::RefreshMovingStaticObstacles,
			StaticObstacleRefreshInterval,
			true);
	}
}

void UNavigationSubsystem::SetRuntimeNavObstacleActors(const TArray<AActor*>& Actors)
{
	RuntimeNavObstacles.Reset();

	for (AActor* Actor : Actors)
	{
		FNavObstacleSphereProxy Proxy;
		if (BuildRuntimeObstacleProxy(Actor, Proxy))
		{
			RuntimeNavObstacles.Add(MoveTemp(Proxy));
		}
	}

	RefreshCombinedNavObstacles();
}

const TArray<FNavObstacleSphereProxy>& UNavigationSubsystem::GetStaticNavObstacles() const
{
	EnsureNavObstaclesInitialized();
	return CombinedNavObstacles;
}

bool UNavigationSubsystem::GetClosestObstacleToSegment(const FVector& A, const FVector& B, int32& OutIndex) const
{
	EnsureNavObstaclesInitialized();
	OutIndex = INDEX_NONE;
	float BestDistanceSquared = TNumericLimits<float>::Max();
	const FVector AB = B - A;
	const float ABSizeSquared = AB.SizeSquared();

	for (int32 Index = 0; Index < CombinedNavObstacles.Num(); ++Index)
	{
		const FNavObstacleSphereProxy& Proxy = CombinedNavObstacles[Index];
		const float T = ABSizeSquared <= KINDA_SMALL_NUMBER
			? 0.0f
			: FMath::Clamp(FVector::DotProduct(Proxy.Center - A, AB) / ABSizeSquared, 0.0f, 1.0f);
		const FVector Closest = A + (AB * T);
		const float DistanceSquared = FVector::DistSquared(Proxy.Center, Closest);
		if (DistanceSquared <= FMath::Square(Proxy.InflatedRadius) && DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			OutIndex = Index;
		}
	}

	return OutIndex != INDEX_NONE;
}

bool UNavigationSubsystem::IsSegmentClearOfStaticObstacles(const FVector& A, const FVector& B, int32* OutFirstHitIndex, const AActor* IgnoredObstacleActor) const
{
	EnsureNavObstaclesInitialized();
	if (OutFirstHitIndex)
	{
		*OutFirstHitIndex = INDEX_NONE;
	}

	for (int32 Index = 0; Index < CombinedNavObstacles.Num(); ++Index)
	{
		const FNavObstacleSphereProxy& Proxy = CombinedNavObstacles[Index];
		if (ShouldSkipObstacleForSegment(Proxy, IgnoredObstacleActor))
		{
			continue;
		}
		if (SegmentIntersectsSphere(A, B, Proxy.Center, Proxy.InflatedRadius))
		{
			if (OutFirstHitIndex)
			{
				*OutFirstHitIndex = Index;
			}
			return false;
		}
	}

	return true;
}

const FNavObstacleSphereProxy* UNavigationSubsystem::FindObstacleByActor(const AActor* InActor) const
{
	EnsureNavObstaclesInitialized();
	if (!IsValid(InActor))
	{
		return nullptr;
	}

	for (const FNavObstacleSphereProxy& Proxy : CombinedNavObstacles)
	{
		if (Proxy.Actor.Get() == InActor)
		{
			return &Proxy;
		}
	}

	return nullptr;
}

bool UNavigationSubsystem::ShouldSkipObstacleForSegment(const FNavObstacleSphereProxy& Proxy, const AActor* IgnoredActor) const
{
	return IsValid(IgnoredActor) && Proxy.Actor.Get() == IgnoredActor;
}

bool UNavigationSubsystem::FindLineTraceNavStaticBigObstacle(const FVector& A, const FVector& B, const AActor* IgnoredActor, int32* OutObstacleIndex) const
{
	if (OutObstacleIndex)
	{
		*OutObstacleIndex = INDEX_NONE;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NavStaticBigLineTrace), false);
	QueryParams.bReturnPhysicalMaterial = false;

	TArray<FHitResult> Hits;
	if (!World->LineTraceMultiByObjectType(Hits, A, B, ObjectQueryParams, QueryParams))
	{
		if (bNavDebugDrawLineTrace)
		{
			DrawDebugLine(World, A, B, FColor::Green, false, 1.0f, 0, 2.0f);
		}
		return false;
	}

	for (const FHitResult& Hit : Hits)
	{
		AActor* HitNavActor = FindNavStaticBigOwner(Hit.GetActor());
		if (!IsValid(HitNavActor) || HitNavActor == IgnoredActor)
		{
			continue;
		}

		for (int32 Index = 0; Index < CombinedNavObstacles.Num(); ++Index)
		{
			if (CombinedNavObstacles[Index].Actor.Get() == HitNavActor)
			{
				if (bNavDebugDrawLineTrace)
				{
					DrawDebugLine(World, A, B, FColor::Red, false, 1.0f, 0, 2.5f);
					DrawDebugPoint(World, Hit.ImpactPoint, 18.0f, FColor::Red, false, 1.0f);
				}
				if (OutObstacleIndex)
				{
					*OutObstacleIndex = Index;
				}
				return true;
			}
		}
	}

	if (bNavDebugDrawLineTrace)
	{
		DrawDebugLine(World, A, B, FColor::Green, false, 1.0f, 0, 2.0f);
	}

	return false;
}

bool UNavigationSubsystem::IsPointInsideNavStaticBigProxy(const FVector& Point, const AActor* IgnoredActor, int32* OutObstacleIndex) const
{
	EnsureNavObstaclesInitialized();
	if (OutObstacleIndex)
	{
		*OutObstacleIndex = INDEX_NONE;
	}

	for (int32 Index = 0; Index < CombinedNavObstacles.Num(); ++Index)
	{
		const FNavObstacleSphereProxy& Proxy = CombinedNavObstacles[Index];
		if (ShouldSkipObstacleForSegment(Proxy, IgnoredActor))
		{
			continue;
		}

		if (FVector::DistSquared(Point, Proxy.Center) <= FMath::Square(Proxy.InflatedRadius))
		{
			if (OutObstacleIndex)
			{
				*OutObstacleIndex = Index;
			}
			if (bNavDebugDrawLineTrace)
			{
				if (UWorld* World = GetWorld())
				{
					DrawDebugPoint(World, Point, 24.0f, FColor::Orange, false, 1.0f);
					DrawDebugSphere(World, Proxy.Center, Proxy.InflatedRadius, 24, FColor::Orange, false, 1.0f, 0, 1.5f);
				}
			}
			return true;
		}
	}

	return false;
}

bool UNavigationSubsystem::IsDirectPathBlockedByNavStaticBig(const FVector& A, const FVector& B, const AActor* IgnoredActor, int32* OutObstacleIndex) const
{
	EnsureNavObstaclesInitialized();
	if (IsPointInsideNavStaticBigProxy(A, IgnoredActor, OutObstacleIndex))
	{
		return true;
	}
	return FindLineTraceNavStaticBigObstacle(A, B, IgnoredActor, OutObstacleIndex);
}

FVector UNavigationSubsystem::ResolveEffectiveGoalForTargetObstacle(const FVector& Start, const FVector& RequestedGoal, const AActor* TargetActor) const
{
	const FNavObstacleSphereProxy* TargetProxy = FindObstacleByActor(TargetActor);
	if (!TargetProxy)
	{
		return RequestedGoal;
	}

	const FVector FromTargetToStart = (Start - TargetProxy->Center).GetSafeNormal();
	FVector BestPoint = TargetProxy->Center + FromTargetToStart * (TargetProxy->InflatedRadius + 1.0f);
	float BestScore = TNumericLimits<float>::Max();

	for (const FVector& Anchor : TargetProxy->Anchors)
	{
		if (!IsSegmentClearOfStaticObstacles(Start, Anchor, nullptr, TargetActor))
		{
			continue;
		}

		const FVector FromTargetToAnchor = (Anchor - TargetProxy->Center).GetSafeNormal();
		const float SideScore = FVector::DotProduct(FromTargetToAnchor, FromTargetToStart);
		const float Score = FVector::DistSquared(Start, Anchor) - (SideScore * FMath::Square(TargetProxy->InflatedRadius));
		if (Score < BestScore)
		{
			BestScore = Score;
			BestPoint = Anchor;
		}
	}

	return BestPoint;
}

bool UNavigationSubsystem::DebugTestSegmentAgainstStaticObstacles(const FVector& A, const FVector& B)
{
	EnsureNavObstaclesInitialized();
	const bool bIsClear = IsSegmentClearOfStaticObstacles(A, B, nullptr);
	if (bNavDebugDrawStatic)
	{
		if (UWorld* World = GetWorld())
		{
			constexpr float DebugDuration = 5.0f;
			const FColor LineColor = bIsClear ? FColor::Green : FColor::Red;
			DrawDebugLine(World, A, B, LineColor, false, DebugDuration, 0, 2.0f);
		}
	}

	return bIsClear;
}

int32 UNavigationSubsystem::GetStaticNavObstacleCount() const
{
	EnsureNavObstaclesInitialized();
	return StaticNavObstacles.Num();
}

void UNavigationSubsystem::DebugDrawStaticNavObstacles(float Duration) const
{
	EnsureNavObstaclesInitialized();
	DrawStaticNavObstacles(Duration);
}

void UNavigationSubsystem::DrawStaticNavObstacles(float Duration) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float SafeDuration = FMath::Max(Duration, 0.0f);
	for (const FNavObstacleSphereProxy& Proxy : StaticNavObstacles)
	{
		DrawDebugSphere(World, Proxy.Center, Proxy.InflatedRadius, 32, FColor::Cyan, false, SafeDuration, 0, 2.0f);
		for (const FVector& Anchor : Proxy.Anchors)
		{
			DrawDebugPoint(World, Anchor, 16.0f, FColor::Yellow, false, SafeDuration);
		}
	}
}

TArray<FVector> UNavigationSubsystem::FindGlobalPathAnchors(const FVector& Start, const FVector& Goal, AActor* TargetActor) const
{
	EnsureNavObstaclesInitialized();
	TArray<FVector> Result;
	const AActor* IgnoredTargetObstacle = IsValid(TargetActor) && FindObstacleByActor(TargetActor) ? TargetActor : nullptr;
	const FVector EffectiveGoal = ResolveEffectiveGoalForTargetObstacle(Start, Goal, TargetActor);
	int32 TraceBlockedObstacleIndex = INDEX_NONE;
	const bool bTraceBlockedByNavStaticBig = FindLineTraceNavStaticBigObstacle(Start, EffectiveGoal, IgnoredTargetObstacle, &TraceBlockedObstacleIndex);
	int32 ContainingObstacleIndex = INDEX_NONE;
	const bool bStartsInsideNavStaticBig = IsPointInsideNavStaticBigProxy(Start, IgnoredTargetObstacle, &ContainingObstacleIndex);
	if (bStartsInsideNavStaticBig && TraceBlockedObstacleIndex == INDEX_NONE)
	{
		TraceBlockedObstacleIndex = ContainingObstacleIndex;
	}

	if (!bTraceBlockedByNavStaticBig && !bStartsInsideNavStaticBig && IsSegmentClearOfStaticObstacles(Start, EffectiveGoal, nullptr, IgnoredTargetObstacle))
	{
		Result.Add(EffectiveGoal);
		if (bNavDebugDrawStatic)
		{
			if (UWorld* World = GetWorld())
			{
				constexpr float DebugDuration = 3.0f;
				DrawDebugLine(World, Start, EffectiveGoal, FColor::Green, false, DebugDuration, 0, 2.0f);
			}
		}
		return Result;
	}

	constexpr int32 MaxAnchors = 2000;
	constexpr int32 MaxWaypoints = 8;
	constexpr float CorridorPaddingCm = 2000.0f;

	TArray<const FNavObstacleSphereProxy*> CandidateObstacles;
	CandidateObstacles.Reserve(CombinedNavObstacles.Num());
	for (const FNavObstacleSphereProxy& Proxy : CombinedNavObstacles)
	{
		const float CorridorRadius = Proxy.InflatedRadius + CorridorPaddingCm;
		if (SegmentIntersectsSphere(Start, EffectiveGoal, Proxy.Center, CorridorRadius))
		{
			CandidateObstacles.Add(&Proxy);
		}
	}
	if (CombinedNavObstacles.IsValidIndex(TraceBlockedObstacleIndex))
	{
		CandidateObstacles.AddUnique(&CombinedNavObstacles[TraceBlockedObstacleIndex]);
	}

	auto BuildFallbackAnchorPath = [&]()
	{
		TArray<FVector> FallbackResult;
		if (!CombinedNavObstacles.IsValidIndex(TraceBlockedObstacleIndex))
		{
			return FallbackResult;
		}

		const FNavObstacleSphereProxy& BlockedProxy = CombinedNavObstacles[TraceBlockedObstacleIndex];
		const AActor* BlockedActor = BlockedProxy.Actor.Get();
		const int32 AnchorCount = BlockedProxy.Anchors.Num();
		if (AnchorCount == 0)
		{
			return FallbackResult;
		}

		auto IsSegmentUsableAroundBlockedObstacle = [&](const FVector& A, const FVector& B, const AActor* TraceIgnoredActor)
		{
			return IsSegmentClearOfStaticObstacles(A, B, nullptr, BlockedActor)
				&& !FindLineTraceNavStaticBigObstacle(A, B, TraceIgnoredActor, nullptr);
		};

		auto BuildAnchorSequence = [&](int32 EntryIndex, int32 ExitIndex, int32 Direction)
		{
			TArray<int32> Sequence;
			if (!BlockedProxy.Anchors.IsValidIndex(EntryIndex) || !BlockedProxy.Anchors.IsValidIndex(ExitIndex) || Direction == 0)
			{
				return Sequence;
			}

			Sequence.Add(EntryIndex);
			int32 CurrentIndex = EntryIndex;
			int32 Safety = 0;
			while (CurrentIndex != ExitIndex && Safety < AnchorCount)
			{
				const int32 NextIndex = (CurrentIndex + Direction + AnchorCount) % AnchorCount;
				if (!IsSegmentUsableAroundBlockedObstacle(BlockedProxy.Anchors[CurrentIndex], BlockedProxy.Anchors[NextIndex], nullptr))
				{
					Sequence.Reset();
					return Sequence;
				}
				Sequence.Add(NextIndex);
				CurrentIndex = NextIndex;
				++Safety;
			}

			if (CurrentIndex != ExitIndex)
			{
				Sequence.Reset();
			}
			return Sequence;
		};

		auto ScoreSequence = [&](const TArray<int32>& Sequence)
		{
			float Score = 0.0f;
			FVector PrevPoint = Start;
			for (const int32 AnchorIndex : Sequence)
			{
				const FVector& Anchor = BlockedProxy.Anchors[AnchorIndex];
				Score += FVector::Dist(PrevPoint, Anchor);
				PrevPoint = Anchor;
			}
			Score += FVector::Dist(PrevPoint, EffectiveGoal);
			return Score;
		};

		TArray<int32> BestSequence;
		float BestScore = TNumericLimits<float>::Max();

		for (int32 EntryIndex = 0; EntryIndex < AnchorCount; ++EntryIndex)
		{
			const FVector& EntryAnchor = BlockedProxy.Anchors[EntryIndex];
			if (!IsSegmentUsableAroundBlockedObstacle(Start, EntryAnchor, nullptr))
			{
				continue;
			}

			for (int32 ExitIndex = 0; ExitIndex < AnchorCount; ++ExitIndex)
			{
				const FVector& ExitAnchor = BlockedProxy.Anchors[ExitIndex];
				if (!IsSegmentClearOfStaticObstacles(ExitAnchor, EffectiveGoal, nullptr, IgnoredTargetObstacle)
					|| FindLineTraceNavStaticBigObstacle(ExitAnchor, EffectiveGoal, IgnoredTargetObstacle, nullptr))
				{
					continue;
				}

				TArray<int32> ForwardSequence = BuildAnchorSequence(EntryIndex, ExitIndex, 1);
				if (ForwardSequence.Num() > 0)
				{
					const float Score = ScoreSequence(ForwardSequence);
					if (Score < BestScore)
					{
						BestScore = Score;
						BestSequence = MoveTemp(ForwardSequence);
					}
				}

				TArray<int32> BackwardSequence = BuildAnchorSequence(EntryIndex, ExitIndex, -1);
				if (BackwardSequence.Num() > 0)
				{
					const float Score = ScoreSequence(BackwardSequence);
					if (Score < BestScore)
					{
						BestScore = Score;
						BestSequence = MoveTemp(BackwardSequence);
					}
				}
			}
		}

		if (BestSequence.Num() == 0)
		{
			int32 BestEntryIndex = INDEX_NONE;
			BestScore = TNumericLimits<float>::Max();
			for (int32 EntryIndex = 0; EntryIndex < AnchorCount; ++EntryIndex)
			{
				const FVector& EntryAnchor = BlockedProxy.Anchors[EntryIndex];
				if (!IsSegmentUsableAroundBlockedObstacle(Start, EntryAnchor, nullptr))
				{
					continue;
				}

				const FVector FromCenter = (EntryAnchor - BlockedProxy.Center).GetSafeNormal();
				const FVector ToGoal = (EffectiveGoal - BlockedProxy.Center).GetSafeNormal();
				const float SideScore = FVector::DotProduct(FromCenter, ToGoal);
				const float Score = FVector::DistSquared(Start, EntryAnchor) - (SideScore * FMath::Square(BlockedProxy.InflatedRadius));
				if (Score < BestScore)
				{
					BestScore = Score;
					BestEntryIndex = EntryIndex;
				}
			}

			if (BestEntryIndex != INDEX_NONE)
			{
				const int32 StepCount = FMath::Min(AnchorCount / 2, MaxWaypoints);
				const int32 Direction = FVector::DotProduct(FVector::CrossProduct(Start - BlockedProxy.Center, EffectiveGoal - BlockedProxy.Center), FVector::UpVector) >= 0.0f ? 1 : -1;
				BestSequence.Add(BestEntryIndex);
				int32 CurrentIndex = BestEntryIndex;
				for (int32 Step = 1; Step < StepCount; ++Step)
				{
					const int32 NextIndex = (CurrentIndex + Direction + AnchorCount) % AnchorCount;
					if (!IsSegmentUsableAroundBlockedObstacle(BlockedProxy.Anchors[CurrentIndex], BlockedProxy.Anchors[NextIndex], nullptr))
					{
						break;
					}
					BestSequence.Add(NextIndex);
					CurrentIndex = NextIndex;
				}
			}
		}

		for (const int32 AnchorIndex : BestSequence)
		{
			if (BlockedProxy.Anchors.IsValidIndex(AnchorIndex))
			{
				FallbackResult.Add(BlockedProxy.Anchors[AnchorIndex]);
			}
		}

		if (bNavDebugDrawLineTrace && FallbackResult.Num() > 0)
		{
			if (UWorld* World = GetWorld())
			{
				FVector From = Start;
				for (int32 Index = 0; Index < FallbackResult.Num(); ++Index)
				{
					const FVector& Point = FallbackResult[Index];
					DrawDebugLine(World, From, Point, FColor::Blue, false, 2.0f, 0, 3.0f);
					DrawDebugPoint(World, Point, Index == 0 ? 28.0f : 20.0f, Index == 0 ? FColor::Blue : FColor::Cyan, false, 2.0f);
					From = Point;
				}
			}
		}

		return FallbackResult;
	};

	if (CandidateObstacles.Num() == 0)
	{
		for (const FNavObstacleSphereProxy& Proxy : CombinedNavObstacles)
		{
			CandidateObstacles.Add(&Proxy);
		}
	}

	int32 AnchorsPerObstacle = NavAnchorsPerObstacle;
	int32 TotalAnchors = 0;
	for (const FNavObstacleSphereProxy* Proxy : CandidateObstacles)
	{
		TotalAnchors += Proxy ? Proxy->Anchors.Num() : 0;
	}

	if (TotalAnchors > MaxAnchors)
	{
		const int32 MaxPerObstacle = FMath::Max(1, MaxAnchors / FMath::Max(1, CandidateObstacles.Num()));
		AnchorsPerObstacle = FMath::Min(AnchorsPerObstacle, MaxPerObstacle);
	}

	TArray<FVector> Nodes;
	Nodes.Reserve(2 + CandidateObstacles.Num() * AnchorsPerObstacle);
	Nodes.Add(Start);
	Nodes.Add(EffectiveGoal);

	for (const FNavObstacleSphereProxy* Proxy : CandidateObstacles)
	{
		if (!Proxy)
		{
			continue;
		}
		const int32 AnchorCount = FMath::Min(AnchorsPerObstacle, Proxy->Anchors.Num());
		for (int32 Index = 0; Index < AnchorCount; ++Index)
		{
			Nodes.Add(Proxy->Anchors[Index]);
		}
	}

	const int32 NodeCount = Nodes.Num();
	if (NodeCount < 2)
	{
		return BuildFallbackAnchorPath();
	}

	struct FPathNode
	{
		float GCost = TNumericLimits<float>::Max();
		float HCost = 0.0f;
		int32 Parent = INDEX_NONE;
		bool bClosed = false;
		bool bOpened = false;
	};

	TArray<FPathNode> NodeData;
	NodeData.SetNum(NodeCount);
	NodeData[0].GCost = 0.0f;
	NodeData[0].HCost = FVector::Dist(Start, EffectiveGoal);
	NodeData[0].bOpened = true;

	TArray<int32> OpenSet;
	OpenSet.Reserve(NodeCount);
	OpenSet.Add(0);

	TMap<uint64, bool> EdgeCache;
	EdgeCache.Reserve(NodeCount * 4);

	constexpr int32 GoalIndex = 1;
	auto IsEdgeClear = [&](int32 IndexA, int32 IndexB)
	{
		if (IndexA == IndexB)
		{
			return false;
		}
		const int32 MinIndex = FMath::Min(IndexA, IndexB);
		const int32 MaxIndex = FMath::Max(IndexA, IndexB);
		const uint64 Key = (static_cast<uint64>(MinIndex) << 32) | static_cast<uint32>(MaxIndex);
		if (const bool* Cached = EdgeCache.Find(Key))
		{
			return *Cached;
		}
		const AActor* IgnoredObstacleForEdge = (IndexA == GoalIndex || IndexB == GoalIndex) ? IgnoredTargetObstacle : nullptr;
		const bool bClear = IsSegmentClearOfStaticObstacles(Nodes[IndexA], Nodes[IndexB], nullptr, IgnoredObstacleForEdge)
			&& !FindLineTraceNavStaticBigObstacle(Nodes[IndexA], Nodes[IndexB], IgnoredObstacleForEdge, nullptr);
		EdgeCache.Add(Key, bClear);
		return bClear;
	};

	bool bFound = false;
	while (OpenSet.Num() > 0)
	{
		int32 BestOpenIndex = 0;
		float BestScore = NodeData[OpenSet[0]].GCost + NodeData[OpenSet[0]].HCost;
		for (int32 Index = 1; Index < OpenSet.Num(); ++Index)
		{
			const int32 NodeIndex = OpenSet[Index];
			const float Score = NodeData[NodeIndex].GCost + NodeData[NodeIndex].HCost;
			if (Score < BestScore)
			{
				BestScore = Score;
				BestOpenIndex = Index;
			}
		}

		const int32 Current = OpenSet[BestOpenIndex];
		OpenSet.RemoveAtSwap(BestOpenIndex);
		NodeData[Current].bClosed = true;

		if (Current == GoalIndex)
		{
			bFound = true;
			break;
		}

		for (int32 Neighbor = 0; Neighbor < NodeCount; ++Neighbor)
		{
			if (Neighbor == Current || NodeData[Neighbor].bClosed)
			{
				continue;
			}
			if (!IsEdgeClear(Current, Neighbor))
			{
				continue;
			}

			const float TentativeG = NodeData[Current].GCost + FVector::Dist(Nodes[Current], Nodes[Neighbor]);
			if (!NodeData[Neighbor].bOpened || TentativeG < NodeData[Neighbor].GCost)
			{
				NodeData[Neighbor].Parent = Current;
				NodeData[Neighbor].GCost = TentativeG;
				NodeData[Neighbor].HCost = FVector::Dist(Nodes[Neighbor], EffectiveGoal);
				if (!NodeData[Neighbor].bOpened)
				{
					NodeData[Neighbor].bOpened = true;
					OpenSet.Add(Neighbor);
				}
			}
		}
	}

	if (!bFound || NodeData[GoalIndex].Parent == INDEX_NONE)
	{
		return BuildFallbackAnchorPath();
	}

	int32 Current = GoalIndex;
	int32 Safety = 0;
	while (Current != 0 && Current != INDEX_NONE && Safety < NodeCount)
	{
		Result.Add(Nodes[Current]);
		Current = NodeData[Current].Parent;
		++Safety;
	}

	Algo::Reverse(Result);
	if (Result.Num() > MaxWaypoints)
	{
		Result.RemoveAt(0, Result.Num() - MaxWaypoints);
	}

	if (bNavDebugDrawStatic)
	{
		if (UWorld* World = GetWorld())
		{
			constexpr float DebugDuration = 3.0f;
			FVector From = Start;
			for (const FVector& Point : Result)
			{
				DrawDebugLine(World, From, Point, FColor::Purple, false, DebugDuration, 0, 2.0f);
				From = Point;
			}
		}
	}

	return Result;
}

void UNavigationSubsystem::InitializeNavObstacles()
{
	if (bInitializingStaticNavObstacles)
	{
		return;
	}

	bInitializingStaticNavObstacles = true;
	StaticNavObstacles.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		bStaticNavObstaclesInitialized = false;
		CachedNavWorld = nullptr;
		bInitializingStaticNavObstacles = false;
		return;
	}

	CachedNavWorld = World;
	bStaticNavObstaclesInitialized = true;

	for (TActorIterator<ANavStaticBig> It(World); It; ++It)
	{
		FNavObstacleSphereProxy Proxy;
		if (BuildNavStaticBigProxy(*It, Proxy))
		{
			StaticNavObstacles.Add(MoveTemp(Proxy));
		}
	}

	RefreshCombinedNavObstacles();

	UE_LOG(LogTemp, Log, TEXT("NavigationSubsystem: initialized %d static nav obstacles (%d combined)."),
		StaticNavObstacles.Num(), CombinedNavObstacles.Num());

	if (bNavDebugDrawStatic)
	{
		DrawStaticNavObstacles(15.0f);
	}

	bInitializingStaticNavObstacles = false;
}

void UNavigationSubsystem::RefreshMovingStaticObstacles()
{
	for (FNavObstacleSphereProxy& Proxy : StaticNavObstacles)
	{
		ANavStaticBig* Actor = Cast<ANavStaticBig>(Proxy.Actor.Get());
		FNavObstacleSphereProxy RefreshedProxy;
		if (!BuildNavStaticBigProxy(Actor, RefreshedProxy))
		{
			continue;
		}

		Proxy = MoveTemp(RefreshedProxy);
	}

	RefreshCombinedNavObstacles();
}

bool UNavigationSubsystem::BuildNavStaticBigProxy(ANavStaticBig* Actor, FNavObstacleSphereProxy& OutProxy) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	OutProxy = FNavObstacleSphereProxy();
	OutProxy.Actor = Actor;
	if (Actor->SignatureSphere && Actor->SignatureSphere->GetScaledSphereRadius() > KINDA_SMALL_NUMBER)
	{
		OutProxy.SignatureSphere = Actor->SignatureSphere;
		OutProxy.bFromSignatureSphere = true;
		OutProxy.Center = Actor->SignatureSphere->GetComponentLocation();
		OutProxy.BaseRadius = Actor->SignatureSphere->GetScaledSphereRadius();
	}
	else
	{
		USceneComponent* RootComponent = Actor->GetRootComponent();
		if (!RootComponent)
		{
			return false;
		}
		const FBoxSphereBounds Bounds = RootComponent->Bounds;
		OutProxy.Center = Bounds.Origin;
		OutProxy.BaseRadius = Bounds.SphereRadius;
	}

	OutProxy.InflatedRadius = OutProxy.BaseRadius + DefaultShipRadiusCm + NavSafetyMarginCm;
	GenerateAnchorsForProxy(OutProxy);
	UE_LOG(LogTemp, Verbose, TEXT("NavigationSubsystem: obstacle %s center=%s base=%.1f inflated=%.1f signature=%s anchors=%d"),
		*Actor->GetName(),
		*OutProxy.Center.ToString(),
		OutProxy.BaseRadius,
		OutProxy.InflatedRadius,
		OutProxy.bFromSignatureSphere ? TEXT("true") : TEXT("false"),
		OutProxy.Anchors.Num());
	return true;
}

bool UNavigationSubsystem::BuildRuntimeObstacleProxy(AActor* Actor, FNavObstacleSphereProxy& OutProxy) const
{
	if (!IsValid(Actor))
	{
		return false;
	}

	USceneComponent* RootComponent = Actor->GetRootComponent();
	if (!RootComponent)
	{
		return false;
	}

	const FBoxSphereBounds Bounds = RootComponent->Bounds;
	OutProxy = FNavObstacleSphereProxy();
	OutProxy.Actor = Actor;
	OutProxy.Center = Bounds.Origin;
	OutProxy.BaseRadius = Bounds.SphereRadius;
	OutProxy.InflatedRadius = OutProxy.BaseRadius + DefaultShipRadiusCm + NavSafetyMarginCm;
	GenerateAnchorsForProxy(OutProxy);
	return true;
}

void UNavigationSubsystem::GenerateAnchorsForProxy(FNavObstacleSphereProxy& Proxy) const
{
	constexpr float GoldenAngle = 2.39996322972865332f;
	const int32 AnchorCount = FMath::Max(NavAnchorsPerObstacle, 1);
	Proxy.Anchors.Reset(AnchorCount);
	Proxy.Anchors.Reserve(AnchorCount);

	for (int32 Index = 0; Index < AnchorCount; ++Index)
	{
		const float T = AnchorCount == 1 ? 0.0f : static_cast<float>(Index) / (AnchorCount - 1);
		const float Z = 1.0f - (2.0f * T);
		const float Radius = FMath::Sqrt(FMath::Max(1.0f - Z * Z, 0.0f));
		const float Angle = GoldenAngle * Index;
		const FVector Direction(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
		Proxy.Anchors.Add(Proxy.Center + Direction * Proxy.InflatedRadius);
	}
}

void UNavigationSubsystem::RefreshCombinedNavObstacles()
{
	CombinedNavObstacles.Reset();
	CombinedNavObstacles.Reserve(StaticNavObstacles.Num() + RuntimeNavObstacles.Num());
	CombinedNavObstacles.Append(StaticNavObstacles);
	CombinedNavObstacles.Append(RuntimeNavObstacles);
}
