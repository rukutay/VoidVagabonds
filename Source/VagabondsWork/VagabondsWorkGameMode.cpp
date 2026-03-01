// Copyright Epic Games, Inc. All Rights Reserved.

#include "VagabondsWorkGameMode.h"

#include "Algo/Reverse.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Math/NumericLimits.h"
#include "PlayerSpectator.h"

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
}

AVagabondsWorkGameMode::AVagabondsWorkGameMode()
{
	DefaultPawnClass = APlayerSpectator::StaticClass();
}

void AVagabondsWorkGameMode::BeginPlay()
{
	Super::BeginPlay();

	InitializeNavObstacles();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			MovingStaticRefreshTimer,
			this,
			&AVagabondsWorkGameMode::RefreshMovingStaticObstacles,
			StaticObstacleRefreshInterval,
			true);
	}
}

void AVagabondsWorkGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MovingStaticRefreshTimer);
	}

	Super::EndPlay(EndPlayReason);
}

const TArray<FNavObstacleSphereProxy>& AVagabondsWorkGameMode::GetStaticNavObstacles() const
{
	return CombinedNavObstacles;
}

bool AVagabondsWorkGameMode::IsSegmentClearOfStaticObstacles(const FVector& A, const FVector& B, int32* OutFirstHitIndex) const
{
	if (OutFirstHitIndex)
	{
		*OutFirstHitIndex = INDEX_NONE;
	}

	for (int32 Index = 0; Index < CombinedNavObstacles.Num(); ++Index)
	{
		const FNavObstacleSphereProxy& Proxy = CombinedNavObstacles[Index];
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

const FNavObstacleSphereProxy* AVagabondsWorkGameMode::FindObstacleByActor(AActor* InActor) const
{
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

void AVagabondsWorkGameMode::SetRuntimeNavObstacleActors(const TArray<AActor*>& Actors)
{
	RuntimeNavObstacles.Reset();
	constexpr float GoldenAngle = 2.39996322972865332f;
	const int32 AnchorCount = FMath::Max(NavAnchorsPerObstacle, 1);

	for (AActor* Actor : Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		USceneComponent* RootComponent = Actor->GetRootComponent();
		if (!RootComponent)
		{
			continue;
		}

		const FBoxSphereBounds Bounds = RootComponent->Bounds;
		FNavObstacleSphereProxy Proxy;
		Proxy.Actor = Actor;
		Proxy.Center = Bounds.Origin;
		Proxy.BaseRadius = Bounds.SphereRadius;
		Proxy.InflatedRadius = Proxy.BaseRadius + DefaultShipRadiusCm + NavSafetyMarginCm;
		Proxy.Anchors.Reserve(AnchorCount);

		for (int32 Index = 0; Index < AnchorCount; ++Index)
		{
			const float T = AnchorCount == 1 ? 0.0f : static_cast<float>(Index) / (AnchorCount - 1);
			const float Z = 1.0f - (2.0f * T);
			const float Radius = FMath::Sqrt(FMath::Max(1.0f - Z * Z, 0.0f));
			const float Angle = GoldenAngle * Index;
			const FVector Direction(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
			const FVector Anchor = Proxy.Center + Direction * (Proxy.InflatedRadius * NavAnchorShellMultiplier);
			Proxy.Anchors.Add(Anchor);
		}

		RuntimeNavObstacles.Add(MoveTemp(Proxy));
	}

	RefreshCombinedNavObstacles();
}

TArray<FVector> AVagabondsWorkGameMode::FindGlobalPathAnchors(const FVector& Start, const FVector& Goal) const
{
	TArray<FVector> Result;

	if (IsSegmentClearOfStaticObstacles(Start, Goal, nullptr))
	{
		Result.Add(Goal);
		if (bNavDebugDrawStatic)
		{
			if (UWorld* World = GetWorld())
			{
				constexpr float DebugDuration = 3.0f;
				DrawDebugLine(World, Start, Goal, FColor::Green, false, DebugDuration, 0, 2.0f);
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
		if (SegmentIntersectsSphere(Start, Goal, Proxy.Center, CorridorRadius))
		{
			CandidateObstacles.Add(&Proxy);
		}
	}

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
	Nodes.Add(Goal);

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
		return Result;
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
	NodeData[0].HCost = FVector::Dist(Start, Goal);
	NodeData[0].bOpened = true;

	TArray<int32> OpenSet;
	OpenSet.Reserve(NodeCount);
	OpenSet.Add(0);

	TMap<uint64, bool> EdgeCache;
	EdgeCache.Reserve(NodeCount * 4);

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
		const bool bClear = IsSegmentClearOfStaticObstacles(Nodes[IndexA], Nodes[IndexB], nullptr);
		EdgeCache.Add(Key, bClear);
		return bClear;
	};

	constexpr int32 GoalIndex = 1;
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
				NodeData[Neighbor].HCost = FVector::Dist(Nodes[Neighbor], Goal);
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
		return Result;
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

void AVagabondsWorkGameMode::InitializeNavObstacles()
{
	StaticNavObstacles.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	constexpr float GoldenAngle = 2.39996322972865332f;
	const int32 AnchorCount = FMath::Max(NavAnchorsPerObstacle, 1);

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || !Actor->ActorHasTag(TEXT("NavStaticBig")))
		{
			continue;
		}

		USceneComponent* RootComponent = Actor->GetRootComponent();
		if (!RootComponent)
		{
			continue;
		}

		const FBoxSphereBounds Bounds = RootComponent->Bounds;
		FNavObstacleSphereProxy Proxy;
		Proxy.Actor = Actor;
		Proxy.Center = Bounds.Origin;
		Proxy.BaseRadius = Bounds.SphereRadius;
		Proxy.InflatedRadius = Proxy.BaseRadius + DefaultShipRadiusCm + NavSafetyMarginCm;
		Proxy.Anchors.Reserve(AnchorCount);

		for (int32 Index = 0; Index < AnchorCount; ++Index)
		{
			const float T = AnchorCount == 1 ? 0.0f : static_cast<float>(Index) / (AnchorCount - 1);
			const float Z = 1.0f - (2.0f * T);
			const float Radius = FMath::Sqrt(FMath::Max(1.0f - Z * Z, 0.0f));
			const float Angle = GoldenAngle * Index;
			const FVector Direction(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), Z);
			const FVector Anchor = Proxy.Center + Direction * (Proxy.InflatedRadius * NavAnchorShellMultiplier);
			Proxy.Anchors.Add(Anchor);
		}

		StaticNavObstacles.Add(MoveTemp(Proxy));
	}

	RefreshCombinedNavObstacles();

	if (bNavDebugDrawStatic)
	{
		constexpr float DebugDuration = 5.0f;
		for (const FNavObstacleSphereProxy& Proxy : StaticNavObstacles)
		{
			DrawDebugSphere(World, Proxy.Center, Proxy.InflatedRadius, 32, FColor::Cyan, false, DebugDuration, 0, 2.0f);
			for (const FVector& Anchor : Proxy.Anchors)
			{
				DrawDebugPoint(World, Anchor, 12.0f, FColor::Yellow, false, DebugDuration);
			}
		}
	}
}

void AVagabondsWorkGameMode::RefreshMovingStaticObstacles()
{
	constexpr float GoldenAngle = 2.39996322972865332f;

	for (FNavObstacleSphereProxy& Proxy : StaticNavObstacles)
	{
		AActor* Actor = Proxy.Actor.Get();
		if (!IsValid(Actor) || !Actor->ActorHasTag(MovingStaticTag))
		{
			continue;
		}

		USceneComponent* Root = Actor->GetRootComponent();
		if (!Root)
		{
			continue;
		}

		const FBoxSphereBounds Bounds = Root->Bounds;
		Proxy.Center = Bounds.Origin;
		Proxy.BaseRadius = Bounds.SphereRadius;
		Proxy.InflatedRadius = Proxy.BaseRadius + DefaultShipRadiusCm + NavSafetyMarginCm;

		const int32 AnchorCount = FMath::Max(NavAnchorsPerObstacle, 1);
		Proxy.Anchors.Reset(AnchorCount);
		Proxy.Anchors.Reserve(AnchorCount);

		for (int32 Index = 0; Index < AnchorCount; ++Index)
		{
			const float T = AnchorCount == 1 ? 0.0f : static_cast<float>(Index) / (AnchorCount - 1);
			const float Z = 1.0f - (2.0f * T);
			const float Rxy = FMath::Sqrt(FMath::Max(1.0f - Z * Z, 0.0f));
			const float Angle = GoldenAngle * Index;
			const FVector Dir(Rxy * FMath::Cos(Angle), Rxy * FMath::Sin(Angle), Z);
			Proxy.Anchors.Add(Proxy.Center + Dir * (Proxy.InflatedRadius * NavAnchorShellMultiplier));
		}
	}

	RefreshCombinedNavObstacles();
}

void AVagabondsWorkGameMode::RefreshCombinedNavObstacles()
{
	CombinedNavObstacles.Reset();
	CombinedNavObstacles.Reserve(StaticNavObstacles.Num() + RuntimeNavObstacles.Num());
	CombinedNavObstacles.Append(StaticNavObstacles);
	CombinedNavObstacles.Append(RuntimeNavObstacles);
}