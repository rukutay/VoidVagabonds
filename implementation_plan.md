# Implementation Plan

[Overview]
Implement patrol progression in `AAIShipController` so a ship consumes `PatrolRoute` points by overlapping each `ANavStaticBig::PlaneRadius`, removes the reached point, waits for a caller-provided delay, and then continues with the new index `0` point.

The current codebase already has route creation (`CreatePatrolRoute`) and navigation steering, but it does not contain patrol progression logic that reacts to overlap with patrol objective volumes. The ship currently navigates to `GetFocusLocation()` (target actor or self), and overlap events are used mainly for avoidance/soft-separation, not patrol objective completion. This gap prevents route consumption and deterministic step-by-step patrol advancement.

The implementation will keep patrol ownership in `AAIShipController::PatrolRoute` (per user confirmation), while integrating progression with existing ship movement flow. `AShip::GetGoalLocation()` will be adjusted to prioritize patrol target when patrol is active, so existing `ShipNavComponent`/steering systems remain intact and timer-driven behavior is preserved. Overlap detection will rely on `AShip::HandleShipRadiusBeginOverlap` because `ANavStaticBig::PlaneRadius` is currently configured with `NoCollision`; the patrol start function will therefore configure route actors’ `PlaneRadius` for query overlap participation to make the mechanic functional without per-tick scans.

[Types]
Add patrol state and timing fields to controller state to support delayed progression.

- `AAIShipController` new/extended state fields:
  - `bool bPatrolActive` — true when an active patrol run exists.
  - `bool bPatrolPauseActive` — true during the post-reach delay window.
  - `float PatrolPointDelaySeconds` — configured delay argument captured at patrol start (validated `>= 0`).
  - `FTimerHandle PatrolResumeTimerHandle` — timer for delayed continuation.
  - `TWeakObjectPtr<ANavStaticBig> CurrentPatrolTarget` (optional helper cache) — avoids repeated route lookups and supports validity checks.

- Validation and behavior rules:
  - Delay argument is clamped to `>= 0.0f`.
  - Reached point is always `PatrolRoute[0]`; on completion it is removed with no shrinking.
  - Invalid/null entries at route head are pruned before resolving goal target.
  - Patrol ends when `PatrolRoute.Num() == 0`; all patrol flags/timers are reset.

[Files]
Modify controller and ship files to add patrol lifecycle, overlap handling integration, and goal sourcing.

- Existing files to be modified:
  - `Source/VagabondsWork/AIShipController.h`
    - Add patrol API for start/progression/cancel/query.
    - Add patrol runtime state fields and timer handle.
  - `Source/VagabondsWork/AIShipController.cpp`
    - Implement start patrol with delay argument.
    - Implement route-head target resolution.
    - Implement point reached handler (remove index 0 + start delay timer).
    - Implement resume callback and cleanup/cancel paths.
  - `Source/VagabondsWork/Ship.h`
    - Add minimal patrol-aware helper declarations if needed for overlap dispatch clarity.
  - `Source/VagabondsWork/Ship.cpp`
    - In `HandleShipRadiusBeginOverlap`, detect `ANavStaticBig` + `PlaneRadius` overlap and notify controller patrol logic.
    - In `GetGoalLocation`, prioritize current patrol point when patrol is active; fall back to existing focus behavior.

- New files to be created: none.
- Files to be deleted/moved: none.
- Configuration updates:
  - None in `.ini`; collision profile changes are applied at runtime to `PlaneRadius` for patrol route actors.

[Functions]
Add patrol lifecycle functions and minimally modify goal/overlap functions.

- New functions in `AAIShipController`:
  - `void StartPatrol(const TArray<ANavStaticBig*>& NavStaticActors, float InPointDelaySeconds);`
    - Builds/stores route via existing `CreatePatrolRoute`, captures delay, enables `PlaneRadius` overlap on route points, and arms patrol state.
  - `ANavStaticBig* GetCurrentPatrolPoint() const;`
    - Returns first valid route point (`index 0`) after pruning invalid head entries.
  - `void HandlePatrolPointOverlap(UPrimitiveComponent* OtherComp, AActor* OtherActor);`
    - Verifies overlap belongs to current route head’s `PlaneRadius`; then consumes point and starts delay timer.
  - `void ResumePatrolAfterDelay();`
    - Clears pause state; ends patrol if route empty.
  - `void StopPatrol(bool bClearRoute);`
    - Clears timer/state and optionally empties route.

- Modified functions:
  - `AAIShipController::BeginPlay()`
    - Ensure patrol timer handles are clean and safe on begin.
  - `AShip::HandleShipRadiusBeginOverlap(...)`
    - Forward overlaps to controller patrol handler before/alongside existing obstacle tracking logic.
  - `AShip::GetGoalLocation() const`
    - Return patrol point location when patrol is active and a valid route head exists.

- Removed functions: none.

[Classes]
Extend existing classes only; no new class introductions.

- Modified classes:
  - `AAIShipController`
    - Gains full patrol progression lifecycle (start, overlap consume, delayed resume, stop).
    - Continues using `PatrolRoute` as authoritative store.
  - `AShip`
    - Becomes the overlap event bridge for patrol objective completion.
    - Uses controller patrol target as navigation goal source when applicable.

- New classes: none.
- Removed classes: none.

[Dependencies]
No external packages are added; only existing Unreal Engine modules/APIs are used.

- UE APIs used/extended:
  - `FTimerManager` / `FTimerHandle` for delayed continuation.
  - `USphereComponent` collision mode/response configuration for `PlaneRadius` overlap viability.
  - Existing controller/ship/nav integration paths already present in module.

[Testing]
Use runtime verification in PIE focused on patrol progression correctness and non-regression.

- Functional tests:
  - Start patrol with a known `NavStaticBig` list and explicit delay argument.
  - Verify first patrol target is `PatrolRoute[0]` location.
  - On ship overlap with that target’s `PlaneRadius`, confirm route shrinks by one (head removed).
  - Confirm ship pauses for provided delay, then resumes toward new `PatrolRoute[0]`.
  - Confirm patrol cleanly stops when route empties.

- Edge-case tests:
  - Delay `0.0` (immediate continuation).
  - Invalid/null actors in input list.
  - Route actor destroyed before being reached (head pruning behavior).
  - Overlap with non-head patrol point should not consume route.

- Regression checks:
  - Existing safety-margin/unstuck behavior still works.
  - Non-patrol goal behavior (`GetFocusLocation`) unchanged when patrol inactive.

[Implementation Order]
Implement controller patrol lifecycle first, then wire ship overlap/goal sourcing, then validate in PIE.

1. Extend `AAIShipController` header with patrol lifecycle APIs/state and timer handle.
2. Implement patrol start/consume/delay/resume/stop logic in `AIShipController.cpp` using `PatrolRoute` index `0` semantics.
3. Update `AShip::HandleShipRadiusBeginOverlap` to dispatch patrol overlap events to controller.
4. Update `AShip::GetGoalLocation` to return patrol head location when patrol is active.
5. Validate end-to-end patrol flow and edge cases in PIE (including delay argument behavior).
