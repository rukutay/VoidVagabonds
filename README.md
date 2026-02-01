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
  spheres, generating tangent temp waypoints when the direct segment is blocked and tracking
  whether temp waypoints come from static vs dynamic avoidance.

## Steering Notes
- `AShip` steers toward the current waypoint from `UShipNavComponent`, which replans from
  `AAIShipController::GetFocusLocation()`.
- `AAIShipController` provides target focus and rotation (no navigation logic).
- `AShip` exposes an instance-editable `TargetActor` (AI|Navigation) used by the controller
  to supply focus for navigation/rotation.
- `AShip` retries controller acquisition during `Tick()` if possession occurs after BeginPlay and
  continues ticking so steering resumes as soon as the controller is available.
- If no `TargetActor` is set, focus falls back to the controlled pawn (no world origin fallback).
- Steering forces apply constant forward thrust scaled by heading with lateral damping.
- Rotation acceleration is tunable via pitch/yaw/roll accel speeds on `AShip` under
  `Ship|AI|Rotation`, with roll disabled so steering aligns via yaw/pitch.
