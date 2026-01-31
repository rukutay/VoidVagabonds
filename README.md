# VagabondsWork

## Overview
VagabondsWork is an Unreal Engine project focused on ship AI and movement behaviors.

## Steering/Avoidance Notes
- Avoidance probes use explicit obstacle groups: static (WorldStatic | WorldDynamic) and
  dynamic (Pawn | PhysicsBody), with avoidance logic branching by hit channel.
- Probe distance now scales with ship speed and mass (braking + turning margins) so heavy
  ships start avoidance earlier.
- Commit/hysteresis is hardened so avoidance keeps a stable side until the commit expires
  or a candidate clearly wins.
- Stuck escape now generates 3–5 waypoints with a ~1.4×ShipRadius acceptance radius to
  walk ships out of near-contact.
- When committing to an avoidance target, the current obstacle actor can be ignored briefly
  (configurable) to prevent infinite ping-ponging, with safety checks for overlaps, start
  penetration, near-contact, and approach direction.
- Debug draws include one-frame probe visualization and current steering target when
  debug flags are enabled.
- Steering forces use smoothed targets and brake interpolation for more natural accel/decel.
- Aggressive seek now requires enabling `bEnableAggressiveSeek` on `AShip` to avoid the
  magnetized pull when targets become visible.
- Rotation acceleration is tunable via pitch/yaw/roll accel speeds on `AShip` under `Ship|AI|Rotation`.
- Rotation derives yaw/pitch errors from the target direction in ship-local space (with
  positive Z targets pitching up) and applies local-space angular velocity converted to world
  space so axes match UE5.
- Ship rotation now disables roll so steering uses only yaw/pitch for alignment.
- Unstuck force now uses a braking-force calculation (based on velocity, mass, and a
  2×ShipRadius stopping distance) to gently prevent direct impacts.
