# Changelog

## [Unreleased]
### Fixed
- Added obstacle group filtering (static: WorldStatic/WorldDynamic, dynamic: Pawn/PhysicsBody)
  so avoidance strategy branches correctly by hit channel.
- Added mass-aware probe distance (braking + turning margins) so heavy ships avoid earlier.
- Hardened commit/hysteresis to prevent left/right flip-flops before commit expiry.
- Added a short ignore window for the currently avoided obstacle to prevent infinite
  avoidance loops, with safety checks for overlaps, start penetration, near-contact, and
  approach direction.
- Expanded stuck escape to 3–5 waypoints with a ~1.4×ShipRadius acceptance radius.
- Added one-frame debug draws for the forward probe and current steering target.
### Changed
- Avoidance helpers now accept static-only/dynamic-only occupancy checks for grid and A*.
