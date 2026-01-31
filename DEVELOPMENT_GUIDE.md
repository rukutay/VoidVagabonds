# Development Guide

## Ship Avoidance
- Avoidance probes use explicit obstacle groups: static (WorldStatic | WorldDynamic) and
  dynamic (Pawn | PhysicsBody), with strategy branching by hit channel.
- Probe distance now scales with ship speed and mass (braking + turning margins) so heavier
  ships start avoiding earlier.
- Commit/hysteresis is hardened to keep left/right choices stable until commit expiry or a
  candidate clearly wins.
- Stuck escape now generates 3–5 waypoints with a ~1.4×ShipRadius acceptance radius to walk
  ships out of near-contact.
- When a ship commits to an avoidance target, it can temporarily ignore the current obstacle
  actor (configurable) to prevent infinite avoidance loops. Safety checks disable ignores
  when overlapping, start-penetrating, near-contact, or still approaching the actor.
- Debug draws include one-frame probe visualization and the current steering target when
  debug flags are enabled.
- `AShip` still smooths steering targets and applies brake interpolation for softer accel/decel.
- Aggressive seek is opt-in via `bEnableAggressiveSeek` to prevent magnetized pull when a
  target becomes visible.
- `AShip` exposes `PitchAccelSpeed`/`YawAccelSpeed`/`RollAccelSpeed` and roll limits under
  `Ship|AI|Rotation` to tune rotation responsiveness and banking behavior.
- Rotation derives yaw/pitch errors from the target direction in local ship axes (positive Z
  target = pitch up), then transforms the resulting angular velocity to world space before
  applying physics.
- Roll steering is disabled in `ApplyShipRotation` so AI ships align using yaw/pitch only.
- `ApplyUnstuckForce` now computes a braking force from current velocity, mass, and a
  2×ShipRadius stopping distance to softly prevent direct hits.
