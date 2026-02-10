# Implementation Plan

[Overview]
Redesign the player ship controller to use the same steering/rotation system as AI while keeping player input responsive and replicating AI tuning on possession changes.

The current player control path uses direct manual rotation input and a separate throttle step system, while AI navigation uses shared steering/rotation with torque PD and angular-velocity servo options. The redesign will unify player steering with the AI steering pipeline so that player-controlled ships follow the same torque-based rotation and tuning values. This will also ensure that when a player possesses a ship, the ship clones the AI controller tuning values for torque PD, rotation limits, and throttle/steering behavior so the feel matches AI handling.

The scope includes modifying the player controller input plumbing to produce steering targets and throttle/rotation inputs compatible with the AI steering system, extending AShip to accept analog throttle input, and ensuring possession swaps replicate all AI tunings into manual control settings. Navigation remains timer-driven for AI; player control will continue to be immediate but will reuse AI rotation math and torque PD settings.

[Types]
No new top-level types are required, but new properties and small data structures will be added for analog throttle and player steering state.

- Add a float property for manual analog throttle input in `AShip` (e.g., `ManualThrottleInput`), clamped [-1, 1].
- Add a float property for current manual throttle target to replace `ManualThrottleStep` logic.
- Add player steering target storage in `APlayerMainController` (e.g., cached look/world target or input vector) if needed for mapping input to target direction.
- Extend `APlayerMainController` to store analog throttle action value and expose it to the ship.

[Files]
Updates will focus on player control and ship steering.

- Modify `Source/VagabondsWork/PlayerMainController.h` and `.cpp`
  - Replace step-based throttle actions with analog throttle input handling.
  - Add input handlers for analog throttle and steering target updates.
  - Route player steering into ship-level steering target/rotation interface shared with AI.
- Modify `Source/VagabondsWork/Ship.h` and `.cpp`
  - Add analog throttle input storage and apply it in manual control.
  - Rework `ApplyManualControl` to use the same steering target path as AI (shared steering + rotation control) while using player input for desired direction.
  - Extend `CacheManualRotationTuning` to copy all relevant AI tuning values (torque PD, max pitch/yaw/roll rates, roll alignment settings, throttle/force, damping, etc.).
  - Ensure possession/unpossession copies and restores tuning values consistently.
- No new files or deletions anticipated.

[Functions]
Player control functions will be added/modified to unify steering.

- New/modified in `APlayerMainController`:
  - `HandleThrottleInput(const FInputActionValue& Value)` to set analog throttle.
  - Update `HandlePitchInput/HandleYawInput/HandleRollInput` to drive a steering target or rotation command consistent with AI rotation (torque PD).
  - `UpdateShipRotationInput()` may be replaced or extended to set a world-space steering target on the ship.
- Modified in `AShip`:
  - `SetManualRotationInput(...)` may change to accept desired look/steer direction or desired angular velocities aligned with AI behavior.
  - `ApplyManualControl(float DeltaTime)` will call `ApplySteeringForce` with a player-derived target location and will call `AAIShipController::ApplyShipRotation` (or shared logic) to use torque-based rotation when enabled.
  - `CacheManualRotationTuning(const AAIShipController* PreviousController)` will copy all AI tunings including torque PD settings, max rates, roll alignment, throttle/damping.
  - Add `SetManualThrottleInput(float Value)` for analog throttle.

[Classes]
No new classes are required; modifications focus on existing controller and ship classes.

- Modified class `APlayerMainController`
  - Add analog throttle input and steering target state.
  - Update input binding logic for throttle and steering inputs.
- Modified class `AShip`
  - Add analog throttle state and use AI steering logic for manual control.
  - Extend possession logic to replicate AI tuning.
- Modified class `AAIShipController`
  - Possibly expose additional getters for tuning values if any required values are currently private and not available to `AShip`.

[Dependencies]
No new dependencies; uses existing Unreal Engine and Enhanced Input modules.

[Testing]
Manual in-editor testing with player possession and AI possession transitions.

- Validate torque-based rotation matches AI behavior while player-controlled.
- Confirm analog throttle is responsive and respects AI tuning values.
- Verify possession/unpossession preserves and restores AI tunings (no drift).
- Check that AI navigation remains timer-driven and unchanged when AI-controlled.

[Implementation Order]
Implement data plumbing first, then behavior changes, then validate possession transitions.

1. Update `APlayerMainController` to capture analog throttle and steering inputs.
2. Extend `AShip` with analog throttle state and update manual control to use AI steering/rotation paths.
3. Expand tuning replication in `CacheManualRotationTuning` and possession handlers.
4. Validate behavior in editor with AI and player possession swaps.
