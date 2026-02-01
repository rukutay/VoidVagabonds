# Development Guide

## Navigation Caching (Static Obstacles)
- `AVagabondsWorkGameMode` gathers actors tagged `NavStaticBig` on BeginPlay and caches them as inflated spheres.
- Inflation uses `DefaultShipRadiusCm` + `NavSafetyMarginCm` to provide a conservative buffer.
- Anchor points are precomputed on a shell (`NavAnchorShellMultiplier`) for future nav sampling.
- Debug drawing can be toggled with `bNavDebugDrawStatic` to visualize cached spheres/anchors.
- `IsSegmentClearOfStaticObstacles` provides a cheap line-of-sight test agai\nst the inflated spheres, and
  `DebugTestSegmentAgainstStaticObstacles` can draw the tested segment (green = clear, red = blocked).
- `FindGlobalPathAnchors` builds a per-query node list from start/goal plus cached anchors, runs A* with
  on-demand line-of-sight edges (cached per query), and returns up to 8 waypoints (goal included).
- When the direct segment is clear, the helper returns only the goal; anchor selection is clamped to
  ~2000 total anchors using a broad corridor prefilter and per-obstacle limits.

## Ship Steering
- `AAIShipController` only provides target focus and rotation (no navigation logic).
- Safety margin tracking uses explicit enter/exit margins: ships enter when distance is below
  `NavSafetyMargin + ShipRadius`, then remain inside until distance exceeds that margin plus the
  hysteresis multiplier, clearing cached nav obstacle references on exit.
- When inside the safety margin, `AShip` uses `ComputeEscapeTarget` as the steering target and
  skips nav target regeneration.
- `AAIShipController::ComputeEscapeTarget` builds a safety-margin escape point using the active
  nav obstacle component or actor center to derive a contact normal, with an optional tangential
  offset.
- `AAIShipController` runs a timer-driven safety margin check that updates `bInsideSafetyMargin`
  based on the closest point on the current obstacle component, preferring the cached nav obstacle
  while already inside the margin and falling back to a proximity query when no overlap exists yet.
- Enable `bDebugSafetyMargin` on `AAIShipController` to draw a one-frame point at proximity-selected
  obstacle contacts for quick visual confirmation.
- `AAIShipController` tracks `bInsideSafetyMargin`/`NavSafetyMargin` and a current nav obstacle
  reference, while still returning the `TargetActor` focus even inside the safety margin.
- `AAIShipController` exits unstuck mode when the timer elapses, speed recovers, or obstacle
  contact is lost, resetting related timers/accumulators so navigation resumes normally.
- `AShip` skips nav target regeneration while the controller reports an active unstuck state to
  avoid oscillating avoidance targets.
- When `bIsUnstucking` is true, `AAIShipController` applies a timer-gated force at the current
  obstacle contact point, scaled by penetration depth and `UnstuckForceStrength`.
- `AAIShipController` exposes `GetCurrentObstacleContactPoint` to query the closest point on the
  current overlapping obstacle component.
- `AAIShipController` runs a lightweight stuck detection timer that accumulates time when speed is
  below `MinStuckSpeed` and distance to goal does not decrease, toggling `bIsUnstucking` when the
  threshold is reached.
- `AAIShipController` stores the current overlapping obstacle component reported by ship radius overlaps.
- `AAIShipController` now exposes initial unstuck state/detection/config variables as placeholders
  (no behavior yet).
- `AShip` exposes an instance-editable `TargetActor` (AI|Navigation) used as the navigation focus.
- `UShipNavComponent` queries `AVagabondsWorkGameMode::FindGlobalPathAnchors` at a replan interval and
  tracks the current waypoint target, then layers a neighbor-based avoidance pass that predicts
  close approaches using each ship's radius to pick temporary waypoints.
- `UShipNavComponent` early-returns when the goal location matches the ship position and treats a
  zeroed nav target as invalid, falling back to the goal location when it is valid.
- `UShipNavComponent` monitors progress toward the active nav target and triggers stuck recovery
  (replan, avoidance boost with decay, temp waypoint cleanup) using ship radius-scaled tolerances.
- `UShipNavComponent` checks cached static obstacle spheres to detect blocked segments and also
  triggers avoidance when the ship is within the inflated radius buffer, generating tangent temp
  waypoints (commit-based) to prevent grazing large objects, tracking whether a temp waypoint is
  static vs dynamic to avoid clearing static avoidance when dynamic avoidance changes.
- With `bDrawNavPath` enabled, proximity-triggered static obstacle checks draw a one-frame sphere
  at the inflated radius used for avoidance.
- `AShip::Tick()` asks the nav component for the current navigation target (dynamic avoidance uses
  a cached neighbor query interval and capped neighbor count for performance).
- `AShip` retries controller acquisition during `Tick()` to handle delayed possession and keeps
  ticking so steering activates as soon as the controller exists.
- If no `TargetActor` exists, focus falls back to the ship pawn location (no player/world origin fallback).
- `AShip` applies forward thrust scaled by heading and lateral damping for baseline movement.
- `AShip` exposes `PitchAccelSpeed`/`YawAccelSpeed`/`RollAccelSpeed` and roll limits under
  `Ship|AI|Rotation` to tune rotation responsiveness and banking behavior.
- Rotation derives yaw/pitch errors from the target direction in local ship axes (positive Z
  target = pitch up), then transforms the resulting angular velocity to world space before
  applying physics.
- Roll steering is disabled in `ApplyShipRotation` so AI ships align using yaw/pitch only.
