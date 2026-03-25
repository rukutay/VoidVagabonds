# Implementation Plan

[Overview]
Implement a fight-specific non-orbit navigation pattern that prevents ramming the current fight target while preserving aggressive attack behavior.

The current crash behavior is caused by one core interaction: `UShipNavComponent::TickNav` treats `IntentTargetActor` as an avoidance-ignore actor in all cases, and in `Fight` mode with `bOrbitTarget=false` this removes the only collision guard against the actor being pursued directly. As a result, steering remains goal-seeking all the way into target collision when no static or dynamic substitute obstacle is selected in time.

The implementation introduces an explicit “attack run + retreat loop” for `Fight` mode when orbiting is disabled. During approach, the ship advances toward a safe attack band and predicts near-term collision with the target volume; when collision risk becomes high, it switches to retreat and steers toward a generated retreat anchor that remains within effective range. Once spacing is restored, it re-enters approach and repeats.

[Types]
Add explicit fight run/retreat phase state in the controller so combat navigation can be phase-driven.

- **New enum** (`Source/VagabondsWork/AIShipController.h`):
  - `UENUM(BlueprintType) enum class EFightRunPhase : uint8`
    - `Approach`
    - `Retreat`
- **New controller state**:
  - `EFightRunPhase FightRunPhase = EFightRunPhase::Approach;`
  - `FVector FightRetreatAnchor = FVector::ZeroVector;`
  - `bool bHasFightRetreatAnchor = false;`
  - `float FightPhaseSwitchCooldownUntil = 0.0f;`
- **New tuning fields**:
  - `FightCollisionPredictTimeSec` (float)
  - `FightTargetSafetyMarginCm` (float)
  - `FightRetreatDistanceMultiplier` (float)
  - `FightApproachReentryMultiplier` (float)
  - `FightMinPhaseHoldSec` (float)

Validation:
- Clamp multipliers/ranges to positive values.
- Reset fight phase state whenever fight target changes or fight exits.

[Files]
Modify only fight/navigation control paths.

- **Modify** `Source/VagabondsWork/AIShipController.h`
  - Add `EFightRunPhase`, state fields, tunables, helper declarations.
- **Modify** `Source/VagabondsWork/AIShipController.cpp`
  - Add phase reset/init in `Fight`, `ResetAction`, `ClearFightTargetState`, destruction handlers.
  - Implement collision prediction, retreat anchor build, phase update, steering goal resolve.
- **Modify** `Source/VagabondsWork/Ship.cpp`
  - In `Tick`, for `Fight && !bOrbitTarget`, resolve phase-based fight goal from controller and pass it into nav/steering/rotation flow.
- **Modify** `Source/VagabondsWork/ShipNavComponent.cpp`
  - Update `ShouldIgnoreActorForAvoidance` so target is not ignored during non-orbit fight-run navigation.
- **Docs to update at end of implementation**:
  - `docs/README.md`
  - `docs/DEVELOPMENT_GUIDE.md`
  - `docs/CHANGELOG.md`

[Functions]
Add focused helpers and adapt existing flow.

- **New (`AAIShipController`)**:
  - `bool ShouldUseFightRunNavigation(const AShip* Ship) const;`
  - `bool PredictFightTargetCollision(const AShip* Ship, const AActor* Target, float PredictTimeSec, float SafetyMarginCm) const;`
  - `FVector BuildFightRetreatAnchor(const AShip* Ship, const AActor* Target) const;`
  - `FVector ResolveFightSteeringGoal(AShip* Ship, float DeltaTime);`
  - `void ResetFightRunState();`
- **Modified**:
  - `AAIShipController::Fight(AActor* TargetActor)`
  - `AAIShipController::ResetAction()`
  - `AAIShipController::ClearFightTargetState()`
  - `AAIShipController::HandleFightTargetDestroyed(AActor*)`
  - `AShip::Tick(float DeltaTime)`
  - `UShipNavComponent::TickNav(float DeltaTime, const FVector& GoalLocation, float ShipRadiusCm, bool bMovingGoal)`

[Classes]
Extend existing classes only.

- **Modified**:
  - `AAIShipController` (fight phase machine + steering goal selection)
  - `AShip` (uses fight-phase steering goal in non-orbit fight mode)
  - `UShipNavComponent` (target-ignore avoidance condition)
- **No new classes**, **no removed classes**.

[Dependencies]
No dependency/package changes are required.

Only internal UE C++ logic is adjusted.

[Testing]
Validate by scenario-based PIE runs.

1. Fight mode + `bOrbitTarget=false` + high-speed approach to stationary target: no collision; retreat loop observed.
2. Fight mode + moving target: repeated approach/retreat cycles without phase flicker.
3. Fight mode + `bOrbitTarget=true`: orbit unchanged.
4. Patrol/Follow/Move regression check.
5. Target destroyed mid-retreat: state reset and restore behavior remains valid.

[Implementation Order]
Implement controller phase logic first, then wire ship/nav integration.

1. Add new enum/state/tunables to `AIShipController.h`.
2. Implement helpers and reset hooks in `AIShipController.cpp`.
3. Integrate non-orbit fight steering-goal resolution in `AShip::Tick`.
4. Refine avoidance ignore logic in `UShipNavComponent::TickNav`.
5. Validate behavior in PIE and tune defaults.
6. Update docs after implementation completion.