# Implementation Plan

[Overview]
Fix ship avoidance deadlocks by allowing thrust to ease off while the ship rotates toward its avoidance target.

Ships can get stuck when a nearby obstacle keeps the physics body pressed into collision, preventing rotation to the avoidance point. The solution is to introduce a short-lived avoidance rotation assist that reduces throttle and increases braking when the avoidance target is at a steep angle while an obstacle is still detected. This keeps the ship from pushing into the obstacle, giving the controller time to rotate cleanly toward the avoidance target.

The change integrates with the existing avoidance loop in `AShip::EvadeThink`, where we already have obstacle hits, avoidance targets, and steering targets. We'll compute a rotation assist state (throttle scale + brake strength) based on the angular error to the current avoidance target and the obstacle hit context. Steering will then use the computed scaling when avoidance is active, without changing aggressive seek behavior when the path is clear.

[Types]
Add avoidance rotation assist tuning and runtime state fields to the ship avoidance configuration.

- **Struct update:** `FObstacleDetectionSettings` (Source/VagabondsWork/Ship.h)
  - `AvoidanceTurnSlowdownAngle` (float, EditAnywhere, default: 45.f)
    - Minimum angle (degrees) between ship forward and avoidance target to trigger slowdown.
  - `AvoidanceTurnThrottleScale` (float, EditAnywhere, default: 0.2f)
    - Throttle multiplier while rotating away from an obstacle (0..1, clamped).
  - `AvoidanceTurnBrakeStrength` (float, EditAnywhere, default: 0.6f)
    - Additional brake strength applied during rotation assist (0..1, clamped).
  - `AvoidanceTurnHoldSeconds` (float, EditAnywhere, default: 0.25f)
    - Duration to keep slowdown active after the last obstacle hit to prevent flicker.
- **Runtime state (AShip):**
  - `float AvoidanceThrottleScale` (private or transient UPROPERTY, default 1.f)
  - `float AvoidanceBrakeScale` (private or transient UPROPERTY, default 0.f)
  - `float LastAvoidanceHitTime` (private, default 0.f)
  - `bool bAvoidanceRotationAssist` (private, default false)

[Files]
Update ship avoidance logic, steering, and documentation to describe the new rotation assist.

- **Modify:** `Source/VagabondsWork/Ship.h`
  - Add the four avoidance rotation assist settings to `FObstacleDetectionSettings`.
  - Add runtime state fields (`AvoidanceThrottleScale`, `AvoidanceBrakeScale`, `LastAvoidanceHitTime`, `bAvoidanceRotationAssist`).
  - Declare a new helper method for calculating the rotation assist state.
- **Modify:** `Source/VagabondsWork/Ship.cpp`
  - Implement the helper method and update `EvadeThink` to set rotation assist values when obstacles are detected.
  - Update `ApplySteeringForce` to apply throttle scaling and brake strength overrides when avoidance assist is active.
- **Modify:** `README.md`
  - Add a note about the new avoidance rotation assist tuning fields and their intended use.
- **Modify:** `DEVELOPMENT_GUIDE.md`
  - Document how rotation assist interacts with avoidance targets and throttling.
- **Modify:** `CHANGELOG.md`
  - Add an Unreleased entry describing the avoidance rotation assist fix.

[Functions]
Add one helper and adjust avoidance and steering functions to honor rotation assist state.

- **New:** `AShip::UpdateAvoidanceRotationAssist`
  - **Signature:** `void UpdateAvoidanceRotationAssist(const FVector& AvoidanceTarget, const FVector& ForwardDir, const FVector& HitLocation, bool bHitObstacle);`
  - **Location:** `Source/VagabondsWork/Ship.cpp`
  - **Purpose:** Compute and cache `AvoidanceThrottleScale`, `AvoidanceBrakeScale`, and `bAvoidanceRotationAssist` based on angular error to the avoidance target and recent obstacle hit timing.
- **Modified:** `AShip::EvadeThink` (Source/VagabondsWork/Ship.cpp)
  - After determining `CurrentTargetLocation` and `bHasAvoidanceTarget`, call `UpdateAvoidanceRotationAssist` using the current avoidance target, ship forward vector, hit location, and `bHitObstacle`.
  - Reset rotation assist state when no obstacle is detected.
- **Modified:** `AShip::ApplySteeringForce` (Source/VagabondsWork/Ship.cpp)
  - When `bHasAvoidanceTarget` and `bAvoidanceRotationAssist` are active, apply `AvoidanceThrottleScale` to `TargetThrottle` and blend `AvoidanceBrakeScale` into `TargetBrakeStrength`.

[Classes]
Only the existing `AShip` class changes to support rotation assist in avoidance.

- **Modified:** `AShip` (Source/VagabondsWork/Ship.h/.cpp)
  - Add avoidance rotation assist settings, runtime state, and helper method.
  - Apply rotation assist during avoidance steering to prevent collisions from blocking rotation.

[Dependencies]
No dependency changes are required.

[Testing]
Validate in-editor with obstacle setups and debug visualization of avoidance targets.

- Use existing debug settings (`ObstacleDebugSettings`) to verify avoidance target selection.
- Place ships near static obstacles and confirm they reduce thrust, rotate away, and resume forward motion.
- Ensure aggressive seek behavior remains unchanged when no obstacle is detected.

[Implementation Order]
Implement avoidance rotation assist state and integrate it into steering before updating docs.

1. Update `FObstacleDetectionSettings` and add `AShip` runtime fields/helper declaration.
2. Implement `UpdateAvoidanceRotationAssist` and integrate it into `EvadeThink`.
3. Apply avoidance throttle/brake scaling in `ApplySteeringForce`.
4. Update `README.md`, `DEVELOPMENT_GUIDE.md`, and `CHANGELOG.md` with the new behavior and tuning options.