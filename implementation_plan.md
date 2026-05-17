# Implementation Plan

[Overview]
Fix ship navigation so `ANavStaticBig` obstacles are always respected by global path planning, including when the target is another actor behind the obstacle and when the target actor itself is an `ANavStaticBig`.

The current navigation stack already centralizes large static obstacle proxies and generated anchor nodes in `UNavigationSubsystem`, while `UShipNavComponent` requests `FindGlobalPathAnchors()` during timer-driven replans and `AShip` steers toward the resulting navigation target. The observed failure is consistent with two existing behaviors: initial path generation is skipped until movement/replan thresholds are met, and global path segment checks treat the destination `ANavStaticBig` as a blocking obstacle even when the ship should approach its signature sphere boundary instead of its center.

The implementation should keep the existing subsystem ownership model: obstacle discovery, path anchor selection, and static segment checks stay in `UNavigationSubsystem`; movement integration remains in `UShipNavComponent`; `AShip` only supplies the correct navigation goal/target actor context. No per-tick heavy pathfinding should be added. Replans remain interval/threshold gated, but the first valid navigation tick must initialize a path immediately.

The high-level approach is to add target-aware global path planning. `UNavigationSubsystem` will receive an optional target actor, ignore that actor only as a terminal obstacle, and use a safe approach point on/near that target obstacle’s inflated shell instead of routing to the obstacle center. `UShipNavComponent` will pass the owning ship’s intent target into the subsystem and force first-time replanning when no waypoint data exists. `AShip` will preserve the existing steering model while allowing NavStaticBig targets to be approached via navigation anchors rather than being treated as direct center goals.

[Types]
Add target-aware planning state without introducing new public gameplay types.

Existing `FNavObstacleSphereProxy` remains the shared obstacle data structure:
- `Actor: TWeakObjectPtr<AActor>` identifies the represented obstacle actor.
- `Center: FVector` is the obstacle proxy center, usually `SignatureSphere` world location for `ANavStaticBig`.
- `BaseRadius: float` is the physical/signature radius before nav inflation.
- `InflatedRadius: float` is `BaseRadius + DefaultShipRadiusCm + NavSafetyMarginCm`.
- `SignatureSphere: TWeakObjectPtr<USphereComponent>` records the source sphere when available.
- `bFromSignatureSphere: bool` records whether signature bounds were used.
- `Anchors: TArray<FVector>` are generated shell points used by global path planning.

No new `USTRUCT`, `UENUM`, or `UCLASS` is required. Function signatures will gain optional actor context:
- `UNavigationSubsystem::IsSegmentClearOfStaticObstacles(...)` should accept an optional ignored obstacle actor for terminal target checks.
- `UNavigationSubsystem::FindGlobalPathAnchors(...)` should accept an optional target actor and derive an effective goal when the target actor is a cached nav obstacle.
- `UShipNavComponent::TickNav(...)` should accept an optional target actor and forward it to global planning.

Validation rules:
- Optional target actor must be considered only when `IsValid(TargetActor)`.
- A target obstacle may be ignored only for final approach/terminal edge checks; unrelated `ANavStaticBig` obstacles must still block segments.
- Effective target approach point must remain outside the target obstacle’s inflated radius with a small positive clearance.
- If no valid target obstacle proxy exists, behavior must match current non-target-aware planning.

[Files]
Modify only the navigation-related source files needed for the fix.

New files to be created:
- None.

Existing files to be modified:
- `Source/VagabondsWork/NavigationSubsystem.h`
  - Add optional ignored actor/target actor parameters to static segment query and global path API declarations.
  - Add private helper declarations for target-aware segment checks and effective target approach point selection.
- `Source/VagabondsWork/NavigationSubsystem.cpp`
  - Update `IsSegmentClearOfStaticObstacles` and internal edge checks to skip a specified ignored obstacle actor when appropriate.
  - Update `FindGlobalPathAnchors` to compute an effective goal when `TargetActor` maps to a `FNavObstacleSphereProxy`.
  - Add helper logic to choose a safe approach point from target obstacle anchors, preferring anchors visible from `Start` and nearest to the requested goal/target side.
  - Ensure returned waypoint array ends in the effective approach point for NavStaticBig targets, not the blocked actor center.
- `Source/VagabondsWork/ShipNavComponent.h`
  - Change `TickNav` signature to accept `AActor* IntentTargetActor` or equivalent optional target context.
  - Add a first-plan state member if existing waypoint/last replan values are insufficient.
- `Source/VagabondsWork/ShipNavComponent.cpp`
  - Forward `IntentTargetActor` into `UNavigationSubsystem::FindGlobalPathAnchors`.
  - Force initial global path request when no valid waypoints have been built yet.
  - Keep local static avoidance behavior unchanged except for compatible signature updates.
- `Source/VagabondsWork/Ship.cpp`
  - Pass `TargetActor` into `ShipNav->TickNav(...)`.
  - Keep current steering target/rotation/thrust model unchanged.
  - Review NavStaticBig arrival logic so it does not reset movement before reaching the computed safe approach/acceptance zone.

Files to be deleted or moved:
- None.

Configuration file updates:
- None.

Documentation updates at end of implementation, after user confirmation per project rules:
- `docs/AI_STATE.md`
- `docs/AI_FILEMAP.md` only if signatures/ownership descriptions need adjustment.
- `docs/README.md`, `docs/DEVELOPMENT_GUIDE.md`, and `docs/CHANGELOG.md` only if the user approves documentation updates after code changes.

[Functions]
Update global path planning and nav component integration functions.

New functions:
- `bool UNavigationSubsystem::ShouldSkipObstacleForSegment(const FNavObstacleSphereProxy& Proxy, const AActor* IgnoredActor) const`
  - File: `Source/VagabondsWork/NavigationSubsystem.cpp`
  - Purpose: centralize target obstacle skip logic for segment tests.
  - Behavior: returns true only when ignored actor is valid and equals `Proxy.Actor.Get()`.
- `FVector UNavigationSubsystem::ResolveEffectiveGoalForTargetObstacle(const FVector& Start, const FVector& RequestedGoal, const AActor* TargetActor) const`
  - File: `Source/VagabondsWork/NavigationSubsystem.cpp`
  - Purpose: for `ANavStaticBig` targets, select a reachable shell/anchor approach point instead of routing to center.
  - Behavior: if target proxy not found, returns `RequestedGoal`; otherwise chooses best anchor/approach point outside the inflated target radius.

Modified functions:
- `bool UNavigationSubsystem::IsSegmentClearOfStaticObstacles(const FVector& A, const FVector& B, int32* OutFirstHitIndex = nullptr) const`
  - File: `Source/VagabondsWork/NavigationSubsystem.h/.cpp`
  - Required change: add optional ignored actor parameter, e.g. `const AActor* IgnoredObstacleActor = nullptr`, and skip that proxy during intersection tests.
- `TArray<FVector> UNavigationSubsystem::FindGlobalPathAnchors(const FVector& Start, const FVector& Goal) const`
  - File: `Source/VagabondsWork/NavigationSubsystem.h/.cpp`
  - Required change: add optional `const AActor* TargetActor = nullptr`; route to effective target approach point if target is a cached obstacle; pass ignored target actor only where needed to prevent target center/shell from invalidating terminal path construction.
- Internal `IsEdgeClear` lambda in `UNavigationSubsystem::FindGlobalPathAnchors`
  - File: `Source/VagabondsWork/NavigationSubsystem.cpp`
  - Required change: use the target-aware segment clear function so candidate graph edges are evaluated consistently.
- `void UShipNavComponent::TickNav(float DeltaTime, const FVector& GoalLocation, float ShipRadiusCm, bool bMovingGoal)`
  - File: `Source/VagabondsWork/ShipNavComponent.h/.cpp`
  - Required change: add `AActor* IntentTargetActor` parameter and use it when calling global path planning.
  - Required change: force the first path build when `GlobalWaypoints.Num() == 0` and no initial plan has been recorded.
- `void AShip::Tick(float DeltaTime)` navigation section
  - File: `Source/VagabondsWork/Ship.cpp`
  - Required change: pass `TargetActor` to `ShipNav->TickNav(...)`.

Removed functions:
- None.

[Classes]
No new classes are required; only existing navigation classes are extended.

New classes:
- None.

Modified classes:
- `UNavigationSubsystem`
  - File: `Source/VagabondsWork/NavigationSubsystem.h/.cpp`
  - Specific modifications: target-aware segment checking, target obstacle approach-point resolution, and optional target actor support in `FindGlobalPathAnchors`.
- `UShipNavComponent`
  - File: `Source/VagabondsWork/ShipNavComponent.h/.cpp`
  - Specific modifications: pass target context into the subsystem and ensure first path request is not blocked by replan thresholds.
- `AShip`
  - File: `Source/VagabondsWork/Ship.cpp`
  - Specific modifications: pass `TargetActor` into nav component; preserve existing steering, safety margin, and unstuck behavior.

Removed classes:
- None.

[Dependencies]
No dependency changes are required.

No new packages, plugins, modules, or Unreal Build.cs dependencies are needed. Existing dependencies already include `Engine`, `AIModule`, and project navigation headers. Header changes should use forward declarations where possible (`class AActor;`) and avoid adding heavy includes.

[Testing]
Validate with targeted in-editor scenarios and optional log/debug draw checks; do not compile unless the user permits.

Manual validation scenarios:
- Place a ship, a target actor, and an `ANavStaticBig` between them. Command ship to move to target. Expected: `UShipNavComponent` receives global waypoints from `UNavigationSubsystem`, steering target becomes an anchor/shell path, and ship moves around obstacle rather than directly through/into it.
- Command ship to move to a target that is itself an `ANavStaticBig`. Expected: path routes to a safe approach anchor/shell point outside the signature sphere/inflated radius and movement does not immediately fail due to target sphere intersection.
- Verify direct unobstructed movement still returns a single goal waypoint and does not add unnecessary detours.
- Verify local static avoidance remains disabled by default unless explicitly enabled; global anchor planning should solve the described issue without per-tick static avoidance.
- If `bDrawNavPath` or `bNavDebugDrawStatic` is enabled, confirm debug lines show purple/cyan path around the obstacle and yellow anchor points around NavStaticBig proxies.

Code validation:
- Inspect all call sites of `TickNav`, `FindGlobalPathAnchors`, and `IsSegmentClearOfStaticObstacles` after signature changes.
- Ensure no per-tick full graph pathfinding is introduced beyond existing replan gates.
- Do not run compilation unless the user explicitly grants permission.

[Implementation Order]
Implement the fix from subsystem outward, then update call sites and validate signatures.

1. Update `UNavigationSubsystem` declarations in `Source/VagabondsWork/NavigationSubsystem.h` for optional target/ignored actor context.
2. Implement target-aware obstacle skipping and effective NavStaticBig approach-point selection in `Source/VagabondsWork/NavigationSubsystem.cpp`.
3. Update `FindGlobalPathAnchors` graph construction and edge checks to use the effective goal and target-aware segment tests.
4. Update `UShipNavComponent::TickNav` signature and implementation to pass `IntentTargetActor` and force the initial path request.
5. Update `AShip::Tick` navigation call to pass `TargetActor` into `ShipNav->TickNav`.
6. Search all affected function names to update every call site and ensure signatures are coherent.
7. Perform non-compiling static validation by reviewing the edited diff and searching for stale signatures.
8. Ask user whether to update documentation files and whether they want a compile/test run.
