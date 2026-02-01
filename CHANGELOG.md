# Changelog

## [Unreleased]
### Changed
- Hardened `UShipNavComponent` to avoid returning zero nav targets when a goal is valid and to early-return when already at the goal.
- Added debug logs/on-screen messaging when a ship has no valid target (goal equals self).
- Exposed `AShip::TargetActor` for per-instance Blueprint assignment and moved focus sourcing
  in `AAIShipController` to read the pawn's target actor.
- Added static navigation obstacle caching in `AVagabondsWorkGameMode`, inflating tagged actors
  into spheres with precomputed anchor points for future navigation work.
- Added a segment-vs-sphere line-of-sight check for cached static obstacles, plus a debug
  helper to draw tested segments.
- Added `FindGlobalPathAnchors` to build a per-query node graph from cached anchors and run
  a lightweight A* pass for global waypoint selection with debug drawing.
- Added `UShipNavComponent` to replan global waypoints from the game mode on an interval and
  provide the current steering target.
- Added ship-to-ship/dynamic avoidance in `UShipNavComponent` using radius-scaled neighbor queries,
  predictive closest-approach tests, and temporary waypoint steering with commit-based side bias.
- Updated `AShip::Tick()` to use `UShipNavComponent` for waypoint steering (no dynamic
  avoidance, grid, or target switching).
- Updated target focus to fall back to the controlled pawn when `TargetActor` is unset.
- Replaced steering force logic with baseline forward thrust + lateral damping.
- Added stuck detection in `UShipNavComponent` that monitors progress toward the nav target using
  ship radius-scaled tolerances, forcing replans and boosting avoidance when recovery is needed.
- Added lightweight static obstacle avoidance in `UShipNavComponent`, detecting blocked segments
  against cached obstacle spheres and generating tangent temp waypoints to bypass them.
- Updated `AShip` controller acquisition to retry in `Tick()` so delayed possession starts
  steering as soon as the controller exists without stopping Tick.
- Updated `AShip::Tick()` steering to always compute goal radius and use `UShipNavComponent`
  when available, with a safe fallback when radius is missing.
- Updated `UShipNavComponent` to use physics linear velocity for physics-simulated ship roots
  instead of relying on `GetOwner()->GetVelocity()`.
