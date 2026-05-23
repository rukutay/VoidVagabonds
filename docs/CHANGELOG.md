# Changelog

User-facing changes only. Technical/internal details live in `VERSION_CHANGES.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).

## 2026-05-23 — Ship scanner refactor & EnemyAppears event
- Replaced `WithinScaner` global `GetAllActorsOfClass` polling with overlap-event-driven membership via `InternalScanerRadius` begin/end overlap handlers. `UpdateWithinScaner` now only prunes invalid/out-of-range actors and re-sorts nearest-first; no longer scans all world actors. Significantly improves scalability toward 500+ ships.
- Added `EnemyAppears(AShip* EnemyShip)` Blueprint-implementable event to `AShip`, fired when scanner detects a new enemy ship (`IsEnemy` check). Triggers on runtime overlap registration and on initial `BeginPlay` seeding; suppressed for duplicate registrations, self, non-ship actors, and neutral/allied ships.

## 2026-05-22 — Ship navigation, AI controller, LevelActorsSubsystem, and content updates
- Added `EnemyShipRadiusBeginOverlap` Blueprint-implementable event to `AShip` that fires when `ShipRadius` begins overlapping an enemy ship's body (`ShipBase`). Only triggers for ships from enemy factions (relation < 0); does not fire for scanner/radius components or non-ship actors. Intended for threat detection / combat state entry in Blueprints.
- Added level visual scheme pipeline:
  - New `ULevelVisualSchemeData` DataAsset for level visual tuning values.
  - New `AVagabondsWorldSettings` with per-level `LevelVisualScheme` reference.
  - New `ULevelVisualSchemeBlueprintLibrary` helpers for safe Blueprint access to world settings and active scheme.
  - Added `SkyboxTexture` (`UTextureCube`) parameter to visual scheme data assets.
- Ship navigation avoidance now ignores the current `TargetActor` (including attached parent/child actor relationships) in all AI action modes, preventing target-chase steering from fighting obstacle avoidance.
- Ship navigation for large static obstacles now runs through `UNavigationSubsystem`, recognizes `ANavStaticBig` planets/stations by class inheritance instead of actor tags, and generates avoidance anchors from each obstacle's `SignatureSphere` scaled world radius.
- Refined ship static avoidance: ships now use greedy reachable `NavStaticBig` anchor paths, periodically shortcut to the target/next anchor when line trace is clear, show all planned anchors in nav debug, and hold a focused static obstacle for a 3s clear-path grace period before returning to normal movement.
- Fixed `NavStaticBig` shortcut traces from inside `SignatureSphere`/atmosphere volumes by ignoring signature components in trace queries while still tracing from the ship position to real blocking geometry.
- AI fight flow now persists across opponent chains: when the current fight target is destroyed, the controller automatically switches to the next closest valid opponent from `CurrentOpponents`.
- AI fight flow now resumes the pre-fight task (`Patroling` / `Moving` / `Following`) only after all opponents are cleared, with fallback state caching to avoid unintended `Idle` transitions.
- Level actor planet queries now return actor lists like station queries, and AI patrol route creation now accepts generic actors instead of only `NavStaticBig` actors.
- AI patrol route creation now uses all provided valid actor candidates and orders them by nearest-neighbor from the ship location instead of selecting a random subset.
- Added Blueprint faction relation list helpers and owner-relative station/planet relation queries for enemy vs neutral/allied filtering.
- Added `GetEnemyShipsOfOwner` and `GetNeutralOrAlliedShipsOfOwner` Blueprint-callable functions to `ULevelActorsSubsystem`, completing the ship owner-relative query set alongside existing station and planet equivalents.

## 2026-05-19 — Station economy & archetype assignment
- Applied `StationArchetype`, `SecurityLevel`, `TradeImportance` to all 10 TestSiteMap stations.
- Replaced station A BPs with station B BPs where appropriate; all replacements use temporary internal names with ActorLabel fix to avoid UE5 name-collision crash.
- Documented supply/demand tables and distance-based price logic per station.

## 2026-05-16 — Editor star system population
- Added matching faction stations on safe planet-orbit offsets and grouped planet/station actors under `PlanetGroups/<PlanetName>` Outliner folders.
