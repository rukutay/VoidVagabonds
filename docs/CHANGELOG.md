# Changelog

User-facing changes only. Technical/internal details live in `VERSION_CHANGES.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).

## [Unreleased]
### Added
- `UFactionsSubsystem` with cache-friendly fixed faction relations matrix (`6x6`, flat `int8` storage, allocation-free).
- Faction relation API in `UFactionsSubsystem`: `GetRelation`, `SetRelation`, `UpdateRelations`, `ResetDefaults`.
- `AAIShipController::EActionMode` enum (`Idle`, `Moving`, `Following`, `Patroling`, `Fight`).
- `AAIShipController` action helpers:
  - `StartFollowing(AShip* TargetShip)`
  - `MoveToTarget(AActor* TargetActor)`
  - `Fight(AActor* TargetActor)`
  - `ResetAction()`
- `ULevelActorsSubsystem` for timer-driven tracking of stations/planets/ships plus faction-filtered query helpers.
- `EMarkerType` entries for `Station` and `Debris`.
- AI movement gating via `bMovementAllowed` (BP-editable) and nearest-neighbor patrol route creation from `NavStaticBig` candidates.
- Player spectator workflow: possession swap, `LookAtActor` helper, smooth camera handoff, and Enhanced Input setup.
- UMG top-down map widget (`UMapWidget`) with player + `NavStaticBig` markers using `LevelBoundaries` radius.
- Runtime atmosphere spawning via `ALevelBoundaries` (predictive placement, overlap-safe checks, capped active count).
- External module system updates: timer-driven aiming (tick disabled), LOS sphere sweep with lead prediction, and expanded firing modes/settings.
- `NavStaticBig` asteroid pipeline: spline generation, near/mid/far HISM streaming, jitter/dropout variation, and near-field actor swap.
- Player manual ship controls + shared yaw-bank tuning (`YawBankScale`) and movement/vitality presets.
- `AShip` Blueprint-readable runtime state booleans: `isMoving`, `isHalfSpeed`, and `IsPlayerLook`.

### Improved
- `AAIShipController::Fight` now propagates the combat target to attached external modules and auto-clears fight state when target actors are destroyed.
- Faction defaults now initialize in subsystem startup: `VoidRaiders` are mutual enemies (`-50`) with all other factions; all other mutual relations remain neutral (`0`), self-relations are `0`.
- Navigation ownership was consolidated from GameMode/GameInstance into subsystems: `UNavigationSubsystem` now owns static/runtime obstacle caches + global anchor pathfinding; `ULevelActorsSubsystem` owns tracked actor lists and faction-filtered queries.
- Actor tracking ownership moved from `UVagabondsGameInstance` to `ULevelActorsSubsystem` (periodic world scans + typed actor/faction queries).
- Following behavior: entering follow disables orbit mode, sets ship target, and matches target speed when within `EffectiveRange`.
- Move behavior: entering move disables orbit mode and auto-resets to idle (and clears target) when within `EffectiveRange`.
- Patrol completion/delay flow: final point now exits patrol cleanly, and delay pauses clear `TargetActor` so ships stop until resume.
- AI roll behavior stabilized: default mode uses shared `YawBankScale` + forward-only passive roll leveling.
- Navigation stability improved with forced replans on stuck/static-blocked states and stronger unstuck obstacle reacquisition.
- Safety margin and local avoidance hardening: ignores invalid/self hits, includes `PhysicsBody` obstacles, and improves predicted closest-approach handling.
- `ShipNavComponent` avoidance now ignores the current intent target actor during avoidance checks (prevents target from being treated as a blocker).
- `NavStaticBig` streaming stability improved with chunk hysteresis, per-chunk budgets, dirty-chunk rebuilds, and more organic spatial jitter/noise.
- `NavStaticBig` asteroid field signature component naming is standardized from `PlaneRadius` to `SignatureSphere`.
- Sun and spectator camera behavior refined (sun tracks active view target; spectator return/look-at handoff is smoother).
- External modules now initialize effective range from owning ship values and use deferred projectile spawning for safer projectile setup.

### Removed
- Roll aim functionality (removed to keep ship behavior stable).

## [Unknown (needs tag/commit reference)]
- Earlier release history is not tagged in this repo. Add versioned entries once tags/commits are identified.
