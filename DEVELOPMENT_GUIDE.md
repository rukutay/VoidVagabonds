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
- `AShip` exposes an instance-editable `TargetActor` (AI|Navigation) used as the navigation focus.
- `UShipNavComponent` queries `AVagabondsWorkGameMode::FindGlobalPathAnchors` at a replan interval and
  tracks the current waypoint target, then layers a neighbor-based avoidance pass that predicts
  close approaches using each ship's radius to pick temporary waypoints.
- `UShipNavComponent` early-returns when the goal location matches the ship position and treats a
  zeroed nav target as invalid, falling back to the goal location when it is valid.
- `UShipNavComponent` monitors progress toward the active nav target and triggers stuck recovery
  (replan, avoidance boost with decay, temp waypoint cleanup) using ship radius-scaled tolerances.
- `UShipNavComponent` checks cached static obstacle spheres to detect blocked segments and generates
  tangent temp waypoints (commit-based) to prevent grazing large objects.
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
