// Fill out your copyright notice in the Description page of Project Settings.


#include "ShipNavComponent.h"
#include "VagabondsWorkGameMode.h"
#include "Ship.h"
#include "AIShipController.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Templates/TypeHash.h"

// Sets default values for this component's properties
UShipNavComponent::UShipNavComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UShipNavComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	AvoidWeightBaseValue = AvoidWeight;
	AvoidWeightBoostUntilTime = 0.0f;
	AvoidWeightBoostStartValue = AvoidWeight;
	NextStuckCheckTime = 0.0f;
	LastDistanceToTarget = 0.0f;
	StuckCounter = 0;
	NextStaticRecheckTime = 0.0f;
	FocusStaticObstacleIndex = INDEX_NONE;
	LastReplanShipPos = FVector::ZeroVector;
	LastReplanGoal = FVector::ZeroVector;
	StaticBlockedAccumTime = 0.0f;
	
}


// Called every frame
void UShipNavComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UShipNavComponent::TickNav(float DeltaTime, const FVector& GoalLocation, float ShipRadiusCm, bool bMovingGoal)
{
	if (!GetWorld() || !GetOwner())
	{
		return;
	}

	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (AvoidWeightBoostUntilTime > 0.0f)
	{
		const float BoostDuration = 2.0f;
		if (CurrentTime >= AvoidWeightBoostUntilTime)
		{
			AvoidWeight = AvoidWeightBaseValue;
			AvoidWeightBoostUntilTime = 0.0f;
			AvoidWeightBoostStartValue = AvoidWeight;
		}
		else
		{
			const float TimeRemaining = FMath::Max(AvoidWeightBoostUntilTime - CurrentTime, 0.0f);
			const float Alpha = 1.0f - FMath::Clamp(TimeRemaining / BoostDuration, 0.0f, 1.0f);
			AvoidWeight = FMath::Lerp(AvoidWeightBoostStartValue, AvoidWeightBaseValue, Alpha);
		}
	}
	
	const FVector OwnerLocation = GetOwner()->GetActorLocation();
	const FVector ShipPos = OwnerLocation;
	AActor* IntentTargetActor = nullptr;
	bool bNonOrbitFightRun = false;
	if (const AShip* OwnerShip = Cast<AShip>(GetOwner()))
	{
		IntentTargetActor = OwnerShip->TargetActor;
		if (const AAIShipController* OwnerController = Cast<AAIShipController>(OwnerShip->GetController()))
		{
			bNonOrbitFightRun = OwnerController->GetActionMode() == EActionMode::Fight
				&& IntentTargetActor != nullptr
				&& !OwnerShip->bOrbitTarget;
		}
	}

	auto ShouldIgnoreActorForAvoidance = [&](const AActor* CandidateActor) -> bool
	{
		if (!IntentTargetActor || !CandidateActor)
		{
			return false;
		}

		if (CandidateActor == IntentTargetActor)
		{
			if (bNonOrbitFightRun)
			{
				return false;
			}
			return true;
		}

		for (const AActor* Parent = CandidateActor->GetAttachParentActor(); Parent; Parent = Parent->GetAttachParentActor())
		{
			if (Parent == IntentTargetActor)
			{
				return true;
			}
		}

		for (const AActor* Parent = IntentTargetActor->GetAttachParentActor(); Parent; Parent = Parent->GetAttachParentActor())
		{
			if (Parent == CandidateActor)
			{
				return true;
			}
		}

		return false;
	};
	
	const bool bTime = (CurrentTime >= NextReplanTime);
	const bool bMoved = FVector::DistSquared(ShipPos, LastReplanShipPos) >
						FMath::Square(ShipRadiusCm * ReplanMinMovedDistanceMultiplier);
	const bool bGoalChanged = (!bMovingGoal) &&
							  (FVector::DistSquared(GoalLocation, LastReplanGoal) >
							   FMath::Square(ShipRadiusCm * ReplanMinGoalDeltaMultiplier));

	const bool bForce = (StuckCounter >= StuckThreshold);

	if (bForce || (bTime && (bMoved || bGoalChanged)))
	{
		if (AVagabondsWorkGameMode* NavigationGameMode = GetWorld()->GetAuthGameMode<AVagabondsWorkGameMode>())
		{
			GlobalWaypoints = NavigationGameMode->FindGlobalPathAnchors(GetOwner()->GetActorLocation(), GoalLocation);
			WaypointIndex = 0;
			LastReplanShipPos = ShipPos;
			LastReplanGoal = GoalLocation;
			NextReplanTime = CurrentTime + FMath::FRandRange(ReplanIntervalMin, ReplanIntervalMax);
		}
	}

	const FVector CurrentWaypoint = (WaypointIndex < GlobalWaypoints.Num())
		? GlobalWaypoints[WaypointIndex]
		: GoalLocation;

	const float AcceptanceRadius = ShipRadiusCm * WaypointAcceptanceRadiusMultiplier;
	if (WaypointIndex < GlobalWaypoints.Num()
		&& FVector::DistSquared(OwnerLocation, CurrentWaypoint) <= FMath::Square(AcceptanceRadius))
	{
		++WaypointIndex;
	}
	if (GoalLocation.Equals(ShipPos, KINDA_SMALL_NUMBER))
	{
		CurrentNavTarget = ShipPos;
		return;
	}
	FVector ShipVelocity = GetOwner()->GetVelocity();
	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
	{
		if (RootPrimitive->IsSimulatingPhysics())
		{
			ShipVelocity = RootPrimitive->GetPhysicsLinearVelocity();
		}
	}
	const float ShipSpeed = ShipVelocity.Size();
	const float NeighborRadius = ShipRadiusCm * NeighborRadiusMultiplier;
	const FVector DesiredTarget = CurrentWaypoint;
	AVagabondsWorkGameMode* NavigationGameMode = GetWorld()->GetAuthGameMode<AVagabondsWorkGameMode>();
	FHitResult TraceHit;
	const FVector ToTargetDir = (DesiredTarget - ShipPos).GetSafeNormal();
	FVector TraceDir = ShipVelocity.GetSafeNormal();
	if (TraceDir.IsNearlyZero())
	{
		TraceDir = ToTargetDir;
	}
	const float TraceBaseDistance = ShipRadiusCm * 4.0f;
	const float TraceDistance = FMath::Clamp((TraceBaseDistance + ShipSpeed * 0.75f)
											  * ForwardTraceDistanceMultiplier,
		TraceBaseDistance,
		ShipRadiusCm * 20.0f);
	int32 StaticHitIndex = INDEX_NONE;
	bool bStaticBlocked = false;
	bool bStaticCheckDue = CurrentTime >= NextStaticRecheckTime;
	if (!bHasTempWaypoint || FocusStaticObstacleIndex == INDEX_NONE)
	{
		bStaticCheckDue = true;
	}
	if (NavigationGameMode && bStaticCheckDue)
	{
		bStaticBlocked = !NavigationGameMode->IsSegmentClearOfStaticObstacles(ShipPos, DesiredTarget, &StaticHitIndex);
		if (bStaticBlocked)
		{
			const TArray<FNavObstacleSphereProxy>& Obstacles = NavigationGameMode->GetStaticNavObstacles();
			if (Obstacles.IsValidIndex(StaticHitIndex)
				&& ShouldIgnoreActorForAvoidance(Obstacles[StaticHitIndex].Actor.Get()))
			{
				bStaticBlocked = false;
				StaticHitIndex = INDEX_NONE;
			}
		}
		if (!bStaticBlocked)
		{
			const TArray<FNavObstacleSphereProxy>& Obstacles = NavigationGameMode->GetStaticNavObstacles();
			float BestDistSq = BIG_NUMBER;
			int32 BestIndex = INDEX_NONE;
			float BestAvoidRadius = 0.0f;
			for (int32 Index = 0; Index < Obstacles.Num(); ++Index)
			{
				const FNavObstacleSphereProxy& Obstacle = Obstacles[Index];
				if (ShouldIgnoreActorForAvoidance(Obstacle.Actor.Get()))
				{
					continue;
				}
				const float AvoidRadius = Obstacle.InflatedRadius + (ShipRadiusCm * StaticAvoidMarginMultiplier);
				const float DistSq = FVector::DistSquared(ShipPos, Obstacle.Center);
				if (DistSq < FMath::Square(AvoidRadius) && DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestIndex = Index;
					BestAvoidRadius = AvoidRadius;
				}
			}
			if (BestIndex != INDEX_NONE)
			{
				bStaticBlocked = true;
				StaticHitIndex = BestIndex;
				if (bDrawNavPath)
				{
					DrawDebugSphere(GetWorld(), Obstacles[BestIndex].Center, BestAvoidRadius, 16,
						FColor::Orange, false, 0.0f, 0, 1.2f);
				}
			}
		}
		NextStaticRecheckTime = CurrentTime + StaticAvoidRecheckInterval;
	}
	else if (FocusStaticObstacleIndex != INDEX_NONE)
	{
		bStaticBlocked = true;
		StaticHitIndex = FocusStaticObstacleIndex;
		if (NavigationGameMode)
		{
			const TArray<FNavObstacleSphereProxy>& Obstacles = NavigationGameMode->GetStaticNavObstacles();
			if (Obstacles.IsValidIndex(StaticHitIndex)
				&& ShouldIgnoreActorForAvoidance(Obstacles[StaticHitIndex].Actor.Get()))
			{
				bStaticBlocked = false;
				StaticHitIndex = INDEX_NONE;
				FocusStaticObstacleIndex = INDEX_NONE;
			}
		}
	}
	bool bStaticAvoidanceActive = false;
	FVector StaticTangentDir = FVector::ZeroVector;
	FVector StaticTempWaypoint = FVector::ZeroVector;
	FVector StaticAvoidCenter = FVector::ZeroVector;
	float StaticAvoidRadius = 0.0f;

	if (bStaticBlocked && NavigationGameMode)
	{
		const TArray<FNavObstacleSphereProxy>& Obstacles = NavigationGameMode->GetStaticNavObstacles();
		if (Obstacles.IsValidIndex(StaticHitIndex))
		{
			const FNavObstacleSphereProxy& Obstacle = Obstacles[StaticHitIndex];
			if (CurrentTime >= CommitUntilTime || FocusStaticObstacleIndex != StaticHitIndex)
			{
				const uint32 Hash = HashCombine(GetTypeHash(GetOwner()), static_cast<uint32>(StaticHitIndex));
				SideSign = (Hash % 2u == 0u) ? 1 : -1;
				CommitUntilTime = CurrentTime + 0.9f;
				FocusStaticObstacleIndex = StaticHitIndex;
			}

			const float AvoidRadius = Obstacle.InflatedRadius + (ShipRadiusCm * StaticAvoidMarginMultiplier);
			const FVector ToTarget = (DesiredTarget - ShipPos).GetSafeNormal();
			FVector FromCenter = ShipPos - Obstacle.Center;
			if (FromCenter.IsNearlyZero())
			{
				FromCenter = -ToTarget * AvoidRadius;
			}
			FVector MoveDir = ShipVelocity.GetSafeNormal();
			if (ShipVelocity.Size() < StaticAvoidMinSpeedCmS)
			{
				MoveDir = GetOwner()->GetActorForwardVector();
			}
			if (MoveDir.IsNearlyZero())
			{
				MoveDir = ToTarget;
			}

			FVector Axis = FVector::CrossProduct(MoveDir, FromCenter).GetSafeNormal();
			if (Axis.IsNearlyZero())
			{
				Axis = FVector::CrossProduct(FVector::UpVector, FromCenter).GetSafeNormal();
			}
			if (Axis.IsNearlyZero())
			{
				Axis = FVector::RightVector;
			}

			StaticTangentDir = FVector::CrossProduct(Axis, FromCenter).GetSafeNormal();
			if (StaticTangentDir.IsNearlyZero())
			{
				StaticTangentDir = ToTarget;
			}

			const int32 StaticSide = SideSign != 0 ? SideSign : 1;
			StaticTangentDir *= static_cast<float>(StaticSide);

			const float ShellRadius = AvoidRadius * StaticAvoidShellMultiplier;
			const FVector PointOnShell = Obstacle.Center + FromCenter.GetSafeNormal() * ShellRadius;
			StaticTempWaypoint = PointOnShell + StaticTangentDir * (ShipRadiusCm * StaticAvoidTempDistMultiplier);
			const FVector ToTemp = (StaticTempWaypoint - ShipPos).GetSafeNormal();
			if (FVector::DotProduct(ToTemp, ToTarget) < 0.2f)
			{
				StaticTempWaypoint = ShipPos
					+ (StaticTangentDir + ToTarget).GetSafeNormal() * (ShipRadiusCm * StaticAvoidTempDistMultiplier);
			}
			StaticAvoidCenter = Obstacle.Center;
			StaticAvoidRadius = AvoidRadius;
			bStaticAvoidanceActive = true;
		}
	}

	if (CurrentTime >= NextNeighborQueryTime)
	{
		TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
		ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_GameTraceChannel3));

		TArray<AActor*> IgnoredActors;
		IgnoredActors.Add(GetOwner());

		TArray<AActor*> Overlaps;
		UKismetSystemLibrary::SphereOverlapActors(
			GetWorld(),
			ShipPos,
			NeighborRadius,
			ObjectTypes,
			AActor::StaticClass(),
			IgnoredActors,
			Overlaps);

		if (Overlaps.Num() > MaxNeighbors)
		{
			Overlaps.SetNum(MaxNeighbors);
		}

		CachedNeighbors.Reset(Overlaps.Num());
		for (AActor* OverlapActor : Overlaps)
		{
			CachedNeighbors.Add(OverlapActor);
		}
		const bool bIdle = Overlaps.Num() == 0 && ShipSpeed <= NeighborQueryIdleSpeedCmS;
		const float Interval = bIdle
			? NeighborQueryInterval * NeighborQueryIdleBackoffMultiplier
			: NeighborQueryInterval;
		NextNeighborQueryTime = CurrentTime + Interval;
	}

	FVector AvoidDir = FVector::ZeroVector;
	float BestPenetration = 0.0f;
	AActor* FocusCandidate = nullptr;
	bool bTraceAvoidance = false;

	if (!TraceDir.IsNearlyZero())
	{
		FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(ShipNavAvoidTrace), false, GetOwner());
		const FVector TraceEnd = ShipPos + TraceDir * TraceDistance;
		const float TraceRadius = FMath::Max(ShipRadiusCm * ForwardTraceRadiusMultiplier, 10.0f);
		const FCollisionShape TraceShape = FCollisionShape::MakeSphere(TraceRadius);
		bTraceAvoidance = GetWorld()->SweepSingleByChannel(
			TraceHit,
			ShipPos,
			TraceEnd,
			FQuat::Identity,
			ECC_GameTraceChannel3,
			TraceShape,
			TraceParams);
		if (bTraceAvoidance && TraceHit.bBlockingHit)
		{
			AActor* TraceActor = TraceHit.GetActor();
			if (ShouldIgnoreActorForAvoidance(TraceActor))
			{
				bTraceAvoidance = false;
			}
			else
			{
			const FVector TraceNormal = TraceHit.ImpactNormal.GetSafeNormal();
			const FVector TraceTangent = (ToTargetDir - FVector::DotProduct(ToTargetDir, TraceNormal) * TraceNormal).GetSafeNormal();
			const FVector TraceAvoidDir = (TraceNormal + TraceTangent * 0.35f).GetSafeNormal();
			if (!TraceAvoidDir.IsNearlyZero())
			{
				AvoidDir += TraceAvoidDir;
			}
			FocusCandidate = TraceActor;

#if !UE_BUILD_SHIPPING
			if (bDrawNavPath)
			{
				DrawDebugLine(GetWorld(), ShipPos, TraceEnd, FColor::Silver, false, 0.0f, 0, 1.0f);
				DrawDebugSphere(GetWorld(), TraceEnd, TraceRadius, 12, FColor::Silver, false, 0.0f, 0, 0.8f);
				DrawDebugPoint(GetWorld(), TraceHit.ImpactPoint, 10.0f, FColor::White, false, 0.0f, 0);
				DrawDebugLine(GetWorld(), TraceHit.ImpactPoint,
					TraceHit.ImpactPoint + TraceNormal * (ShipRadiusCm * 1.5f),
					FColor::White, false, 0.0f, 0, 1.0f);
			}
#endif
			}
		}
	}

	for (const TWeakObjectPtr<AActor>& NeighborPtr : CachedNeighbors)
	{
		AActor* NeighborActor = NeighborPtr.Get();
		if (!NeighborActor || NeighborActor == GetOwner())
		{
			continue;
		}
		if (ShouldIgnoreActorForAvoidance(NeighborActor))
		{
			continue;
		}

		auto IsBlockingNav = [](const UPrimitiveComponent* Comp) -> bool
		{
			if (!Comp || Comp->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				return false;
			}
			const bool bBlocksAvoidance = Comp->GetCollisionResponseToChannel(ECC_GameTraceChannel3) == ECR_Block;
			return bBlocksAvoidance;
		};

		UPrimitiveComponent* BlockingComp = Cast<UPrimitiveComponent>(NeighborActor->GetRootComponent());
		if (!IsBlockingNav(BlockingComp))
		{
			BlockingComp = nullptr;
			TArray<UPrimitiveComponent*> PrimitiveComponents;
			NeighborActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
			for (UPrimitiveComponent* PrimitiveComp : PrimitiveComponents)
			{
				if (IsBlockingNav(PrimitiveComp))
				{
					BlockingComp = PrimitiveComp;
					break;
				}
			}
		}

		FVector NeighborPos = NeighborActor->GetActorLocation();
		FVector NeighborVel = NeighborActor->GetVelocity();
		if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(NeighborActor->GetRootComponent()))
		{
			if (RootPrimitive->IsSimulatingPhysics())
			{
				NeighborVel = RootPrimitive->GetPhysicsLinearVelocity();
			}
		}

		float NeighborRadiusCm = ShipRadiusCm;
		bool bHasShipRadius = false;
		TArray<USphereComponent*> SphereComponents;
		NeighborActor->GetComponents<USphereComponent>(SphereComponents);
		for (USphereComponent* SphereComponent : SphereComponents)
		{
			if (SphereComponent && SphereComponent->GetFName() == TEXT("ShipRadius"))
			{
				NeighborRadiusCm = SphereComponent->GetScaledSphereRadius();
				bHasShipRadius = true;
				break;
			}
		}
		if (NeighborRadiusCm <= 0.0f)
		{
			if (const UPrimitiveComponent* RootComponent = Cast<UPrimitiveComponent>(NeighborActor->GetRootComponent()))
			{
				NeighborRadiusCm = RootComponent->Bounds.SphereRadius * 0.5f;
			}
		}
		if (NeighborRadiusCm <= 0.0f && BlockingComp)
		{
			NeighborRadiusCm = BlockingComp->Bounds.SphereRadius;
		}
		if (NeighborRadiusCm <= 0.0f && !BlockingComp)
		{
			continue;
		}
		if (!bHasShipRadius && BlockingComp)
		{
			NeighborPos = BlockingComp->Bounds.Origin;
		}

		const FVector RelPos = NeighborPos - ShipPos;
		const FVector RelVel = NeighborVel - ShipVelocity;
		const float RelVelSizeSq = RelVel.SizeSquared();
		const float RelSpeed = FMath::Max(RelVel.Size(), 1.0f);
		const float PredictionTime = FMath::Clamp((ShipRadiusCm / RelSpeed) * PredictionTimeMultiplier, 0.5f, 3.0f);

		float TimeToClosest = 0.0f;
		if (RelVelSizeSq > KINDA_SMALL_NUMBER)
		{
			TimeToClosest = FMath::Clamp(-FVector::DotProduct(RelPos, RelVel) / RelVelSizeSq, 0.0f, PredictionTime);
		}

		const FVector Closest = RelPos + RelVel * TimeToClosest;
		const float CombinedRadius = ShipRadiusCm + NeighborRadiusCm + ShipRadiusCm * SeparationMarginMultiplier;
		const float ClosestDistance = Closest.Size();
		if (ClosestDistance < CombinedRadius)
		{
			const float Penetration = CombinedRadius - ClosestDistance;
			AvoidDir += (-Closest).GetSafeNormal() * (Penetration / CombinedRadius);

#if !UE_BUILD_SHIPPING
			if (bDrawNavPath)
			{
				const FVector ClosestWorld = ShipPos + Closest;
				DrawDebugPoint(GetWorld(), ClosestWorld, 10.0f, FColor::Blue, false, 0.0f, 0);
				DrawDebugLine(GetWorld(), ShipPos, ClosestWorld, FColor::Blue, false, 0.0f, 0, 1.0f);
			}
#endif

#if !UE_BUILD_SHIPPING
			if (bDrawNavPath && !bHasShipRadius && BlockingComp)
			{
				DrawDebugSphere(GetWorld(), BlockingComp->Bounds.Origin, NeighborRadiusCm, 12,
					FColor::Magenta, false, 0.0f, 0, 1.0f);
				DrawDebugLine(GetWorld(), ShipPos, BlockingComp->Bounds.Origin, FColor::Magenta,
					false, 0.0f, 0, 1.0f);
			}
#endif
			if (Penetration > BestPenetration)
			{
				BestPenetration = Penetration;
				FocusCandidate = NeighborActor;
			}
		}
	}

	const bool bHasAvoidance = AvoidDir.SizeSquared() > SMALL_NUMBER;
	if (bHasAvoidance && FocusCandidate && !bStaticAvoidanceActive)
	{
		if (CurrentTime >= CommitUntilTime || !FocusActor.IsValid())
		{
			const uint32 Hash = HashCombine(GetTypeHash(GetOwner()), GetTypeHash(FocusCandidate));
			SideSign = (Hash % 2u == 0u) ? 1 : -1;
			CommitUntilTime = CurrentTime + 0.8f;
			FocusActor = FocusCandidate;
		}

		FVector SideVec = FVector::ZeroVector;
		const FVector BasisVelocity = (ShipVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
			? ShipVelocity
			: (CurrentWaypoint - ShipPos);
		if (BasisVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
		{
			SideVec = FVector::CrossProduct(BasisVelocity.GetSafeNormal(), FVector::UpVector).GetSafeNormal();
			AvoidDir += SideVec * static_cast<float>(SideSign) * 0.35f;
		}
	}

	if (bStaticAvoidanceActive)
	{
		const float TempDistanceSq = FVector::DistSquared(ShipPos, StaticTempWaypoint);
		const float TempAcceptSq = FMath::Square(ShipRadiusCm * 2.0f);
		if ((CurrentTime >= CommitUntilTime && !bStaticBlocked) || TempDistanceSq <= TempAcceptSq)
		{
			bStaticAvoidanceActive = false;
			FocusStaticObstacleIndex = INDEX_NONE;
		}
	}

	if (bHasAvoidance || bStaticAvoidanceActive)
	{
		const FVector DesiredDir = (CurrentWaypoint - ShipPos).GetSafeNormal();
		FVector SteerDir = DesiredDir * GoalWeight;
		if (bHasAvoidance)
		{
			SteerDir += AvoidDir * AvoidWeight;
		}
		if (bStaticAvoidanceActive)
		{
			SteerDir += StaticTangentDir * GoalWeight;
		}
		SteerDir = SteerDir.GetSafeNormal();
		if (SteerDir.IsNearlyZero())
		{
			SteerDir = DesiredDir;
		}
		const float TempWaypointDistance = bStaticAvoidanceActive
			? ShipRadiusCm * StaticAvoidTempDistMultiplier
			: ShipRadiusCm * TempWaypointDistanceMultiplier;
		if (bStaticAvoidanceActive)
		{
			TempReason = ETempWaypointReason::Static;
			TempWaypoint = StaticTempWaypoint;
		}
		else if (bHasAvoidance)
		{
			TempReason = ETempWaypointReason::Dynamic;
			TempWaypoint = ShipPos + SteerDir * TempWaypointDistance;
		}
		bHasTempWaypoint = true;
	}
	else if (bHasTempWaypoint)
	{
		const bool bCommitExpired = (CurrentTime >= CommitUntilTime);

		if (TempReason == ETempWaypointReason::Dynamic)
		{
			bool bClearLineOfSight = true;
			FHitResult HitResult;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ShipNavTempLOS), false, GetOwner());
			if (GetWorld()->LineTraceSingleByChannel(HitResult, ShipPos, CurrentWaypoint, ECC_GameTraceChannel3, Params))
			{
				bClearLineOfSight = false;
			}

			if (bCommitExpired || !bHasAvoidance || bClearLineOfSight)
			{
				bHasTempWaypoint = false;
				TempReason = ETempWaypointReason::None;
				FocusActor = nullptr;
			}
		}
		else if (TempReason == ETempWaypointReason::Static)
		{
			bool bStillBlocked = false;

			if (AVagabondsWorkGameMode* NavigationGameModeLocal = GetWorld()->GetAuthGameMode<AVagabondsWorkGameMode>())
			{
				bStillBlocked = !NavigationGameModeLocal->IsSegmentClearOfStaticObstacles(ShipPos, CurrentWaypoint, nullptr);
			}

			if (!bStillBlocked || bCommitExpired)
			{
				bHasTempWaypoint = false;
				TempReason = ETempWaypointReason::None;
				FocusStaticObstacleIndex = INDEX_NONE;
			}
		}
	}

	CurrentNavTarget = bHasTempWaypoint ? TempWaypoint : CurrentWaypoint;

#if !UE_BUILD_SHIPPING
	if (bDebugNav)
	{
		const TCHAR* ReasonText = TEXT("None");
		switch (TempReason)
		{
			case ETempWaypointReason::Dynamic: ReasonText = TEXT("Dynamic"); break;
			case ETempWaypointReason::Static: ReasonText = TEXT("Static"); break;
			default: break;
		}
		UE_LOG(LogTemp, Log,
			TEXT("NavTarget=%s Temp=%s Reason=%s Stuck=%d StaticBlocked=%s"),
			*CurrentNavTarget.ToString(),
			bHasTempWaypoint ? TEXT("Yes") : TEXT("No"),
			ReasonText,
			StuckCounter,
			bStaticBlocked ? TEXT("Yes") : TEXT("No"));
	}
#endif

	// Handle static blocked accumulation and force replan
	if (bStaticBlocked)
	{
		StaticBlockedAccumTime += DeltaTime;
	}
	else
	{
		StaticBlockedAccumTime = 0.0f;
	}

	// Force replan if static blocked for too long
	if (StaticBlockedAccumTime > 1.0f)
	{
		NextReplanTime = CurrentTime;   // force immediate replan
		StaticBlockedAccumTime = 0.0f;
		if (TempReason == ETempWaypointReason::Static)
		{
			bHasTempWaypoint = false;
			TempReason = ETempWaypointReason::None;
			FocusStaticObstacleIndex = INDEX_NONE;
		}
	}
	if (CurrentTime >= NextStuckCheckTime)
	{
		const float MinProgress = ShipRadiusCm * MinProgressMultiplier;
		const float DistToTarget = FVector::Dist(ShipPos, CurrentNavTarget);
		if (LastDistanceToTarget > 0.0f && DistToTarget > (LastDistanceToTarget - MinProgress))
		{
			++StuckCounter;
		}
		else
		{
			StuckCounter = FMath::Max(0, StuckCounter - 1);
		}

		LastDistanceToTarget = DistToTarget;
		NextStuckCheckTime = CurrentTime + StuckCheckInterval;

		if (StuckCounter >= StuckThreshold)
		{
			NextReplanTime = CurrentTime;
			CommitUntilTime = CurrentTime + 1.0f;
			AvoidWeight = AvoidWeightBaseValue * 1.5f;
			AvoidWeightBoostStartValue = AvoidWeight;
			AvoidWeightBoostUntilTime = CurrentTime + 2.0f;
			const float VelDot = FVector::DotProduct(TempWaypoint - ShipPos, ShipVelocity);
			if (bHasTempWaypoint && VelDot < 0.0f)
			{
				bHasTempWaypoint = false;
			}

#if !UE_BUILD_SHIPPING
			if (bDebugNav)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("Nav stuck: Replan forced. VelDot=%.2f"),
					VelDot);
			}
#endif
		}
	}

#if !UE_BUILD_SHIPPING
	if (bDrawNavPath)
	{
		FVector From = OwnerLocation;
		for (int32 Index = WaypointIndex; Index < GlobalWaypoints.Num(); ++Index)
		{
			const FVector& Waypoint = GlobalWaypoints[Index];
			DrawDebugLine(GetWorld(), From, Waypoint, FColor::Cyan, false, 0.0f, 0, 2.0f);
			From = Waypoint;
		}
		DrawDebugPoint(GetWorld(), CurrentNavTarget, 14.0f, FColor::Cyan, false, 0.0f, 0);
		DrawDebugSphere(GetWorld(), ShipPos, ShipRadiusCm, 16, FColor::Green, false, 0.0f, 0, 1.5f);
		DrawDebugSphere(GetWorld(), CurrentNavTarget, ShipRadiusCm * 0.5f, 12, FColor::Purple, false, 0.0f, 0, 1.2f);
		DrawDebugLine(GetWorld(), ShipPos, CurrentNavTarget, FColor::Yellow, false, 0.0f, 0, 1.5f);
		DrawDebugLine(GetWorld(), ShipPos, ShipPos + AvoidDir * ShipRadiusCm * 2.0f, FColor::Red, false, 0.0f, 0, 1.5f);
		if (bStaticAvoidanceActive)
		{
			DrawDebugSphere(GetWorld(), StaticAvoidCenter, StaticAvoidRadius, 16, FColor::Orange, false, 0.0f, 0, 1.2f);
			DrawDebugLine(GetWorld(), ShipPos, DesiredTarget, FColor::Red, false, 0.0f, 0, 1.2f);
			DrawDebugPoint(GetWorld(), StaticTempWaypoint, 12.0f, FColor::Orange, false, 0.0f, 0);
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					reinterpret_cast<uint64>(this) + 1,
					StaticAvoidRecheckInterval,
					FColor::Orange,
					FString::Printf(TEXT("STATIC AVOID idx=%d"), FocusStaticObstacleIndex));
			}
		}
		if (StuckCounter >= StuckThreshold)
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(
					reinterpret_cast<uint64>(this),
					StuckCheckInterval,
					FColor::Red,
					FString::Printf(TEXT("STUCK: %d"), StuckCounter));
			}
		}
	}
#endif
}

FVector UShipNavComponent::GetNavTarget(const FVector& GoalLocation) const
{
	if (bHasTempWaypoint || WaypointIndex < GlobalWaypoints.Num())
	{
		return CurrentNavTarget;
	}

	if (CurrentNavTarget.IsNearlyZero() && !GoalLocation.IsNearlyZero())
	{
		return GoalLocation;
	}

	return GoalLocation;
}

