# VagabondsWork

## Overview
VagabondsWork is an Unreal Engine project focused on ship AI and movement behaviors.

## Navigation Notes
- `AVagabondsWorkGameMode` caches tagged static obstacles (`NavStaticBig`) as inflated spheres at BeginPlay.
- Each cached obstacle stores precomputed anchor points on an inflated shell for future navigation use.
- The game mode exposes a segment-vs-sphere line-of-sight check and a debug helper to draw tested segments.
- `FindGlobalPathAnchors` builds a temporary node list (start/goal + cached anchors), runs A* with
  on-demand line-of-sight edges, and returns up to 8 waypoint anchors (goal included) with debug
  drawing when `bNavDebugDrawStatic` is enabled.
- `UShipNavComponent` asks the game mode for global waypoints on a replan interval and provides the
  current navigation target for steering, layering a lightweight neighbor-based avoidance pass that
  uses ship radius-scaled prediction to generate temporary waypoints.
- When the goal location matches the ship position, `UShipNavComponent` early-returns with the
  current nav target set to the ship position; if the nav target is zero and the goal is valid it
  returns the goal location instead of the origin.
- `UShipNavComponent` also tracks progress toward the current nav target and triggers stuck recovery
  actions (forced replans, avoidance boosts, temp waypoint cleanup) using ship radius-scaled
  tolerances.
- `UShipNavComponent` performs lightweight static obstacle avoidance using the cached obstacle
  spheres, generating tangent temp waypoints when the direct segment is blocked or the ship is
  within the inflated radius buffer, and tracking whether temp waypoints come from static vs
  dynamic avoidance.

## Steering Notes
- `AShip` steers toward the current waypoint from `UShipNavComponent`, which replans from
  `AAIShipController::GetFocusLocation()`.
- `AAIShipController` provides target focus and rotation (no navigation logic).
- `AAIShipController` now runs a proximity overlap query when no current obstacle contact is valid,
  selecting the nearest blocking component within (ShipRadius + NavSafetyMargin).
- `AShip` exposes an instance-editable `TargetActor` (AI|Navigation) used by the controller
  to supply focus for navigation/rotation.
- `AShip` retries controller acquisition during `Tick()` if possession occurs after BeginPlay and
  continues ticking so steering resumes as soon as the controller is available.
- If no `TargetActor` is set, focus falls back to the controlled pawn (no world origin fallback).
- Steering forces apply constant forward thrust scaled by heading with lateral damping.
- Rotation acceleration is tunable via pitch/yaw/roll accel speeds on `AShip` under
  `Ship|AI|Rotation`, with roll disabled so steering aligns via yaw/pitch.

## Soft Separation (Airbag) System
- `AShip` includes a gentle overlap separation system that provides very soft braking on radius overlap and light push only on ShipBase hit events. Force now applies gradual slowdown on overlap and gentle push only when actual collision occurs, eliminating slingshot effects while maintaining smooth avoidance.
- Tunable via Blueprint-exposed parameters under `Ship|SoftSeparation`:
  - `SoftSeparationMarginCm`: Extra margin beyond ship radius for activation (default: 30cm)
  - `SoftSeparationStrength`: Base force scale (default: 40000)
  - `SoftSeparationMaxForce`: Force magnitude clamp (default: 90000)
  - `SoftSeparationDamping`: Reduces relative velocity into obstacles (default: 2.0)
  - `SoftSeparationResponse`: Force smoothing interpolation speed (default: 6.0)
  - `SoftSeparationMaxActors`: Maximum overlapping components processed per tick (default: 6)
  - `bDebugSoftSeparation`: Enable debug visualization (yellow sphere + blue force vector)
- Uses relative velocity for ship-to-ship interactions for more natural collision response
- Performance-optimized with periodic cleanup and efficient top-N overlap selection
