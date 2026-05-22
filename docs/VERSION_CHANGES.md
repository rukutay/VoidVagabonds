# Version Changes

Technical/internal changes only. User-facing summaries live in `CHANGELOG.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [CHANGELOG.md](CHANGELOG.md).

## 2026-05-22 — Ship navigation, AI controller, LevelActorsSubsystem, and content updates

### Navigation / Avoidance
- Timer-driven global replanning with cached static obstacles owned by `UNavigationSubsystem`.
- Static obstacle discovery uses `TActorIterator<ANavStaticBig>` (class/children) instead of actor tags.
- Greedy reachable-anchor pathfinding with periodic line-trace shortcuts and 3s focused-obstacle grace period.
- `NavStaticBig` shortcut traces ignore `SignatureSphere` components so ships inside signature volumes still detect real body blockers.
- `UShipNavComponent::TickNav` ignores the current `TargetActor` (including attach parent/child) in all action modes, not only Fight.
- `bEnableLocalStaticAvoidance` toggle and `StaticPathShortcutCheckInterval` (5s) for static-path shortening checks.
- Ship neighbor avoidance uses repulsion at predicted closest approach with relative-speed prediction; includes dynamic/awakened asteroid actors (WorldDynamic) and blocking `PhysicsBody` components.

### Steering / Physics
- Forward thrust + yaw/pitch rotation model (roll disabled).
- Soft-separation (airbag) overlap response for gentle braking/repulsion.
- Shared `YawBankScale` for AI + player; AI forward-only passive roll leveling.

### AI Controller (`AAIShipController`)
- Action mode enum: `Idle`, `Moving`, `Following`, `Patroling`, `Fight`, `Flee`.
- Fight mode pushes target to controlled ship + attached `AExternalModule` children; auto-chains to next closest opponent from `CurrentOpponents` on target destruction.
- Post-fight resume uses suspended-state fallback and last-task cache to avoid unintended `Idle`.
- `bMovementAllowed` gates AI movement/rotation.
- Deterministic nearest-neighbor patrol route creation from generic actor candidates.
- Following speed-matching and move-to-target arrival auto-reset.
- Patrol overlap consumes point, enters delay with `TargetActor = nullptr`, and auto-stops on route exhaustion.
- Unstuck recovery with obstacle reacquisition, penetration-scaled force, and escape-target steering.
- Safety margin escape with tangent offsets, fallback guard, and suppress-on-Nav-fallback logic.

### LevelActorsSubsystem
- Owns periodic-refresh `Stations`/`Planets`/`Ships` tracked lists.
- `GetStationsAll()`/`GetPlanetsAll()`/`GetShipsAll()`.
- `GetStationsOfFaction()`/`GetPlanetsOfFaction()`/`GetShipsOfFaction()`.
- Owner-relative relation queries: `GetEnemyStationsOfOwner`/`GetNeutralOrAlliedStationsOfOwner`, `GetEnemyPlanetsOfOwner`/`GetNeutralOrAlliedPlanetsOfOwner`, `GetEnemyShipsOfOwner`/`GetNeutralOrAlliedShipsOfOwner`.
- Relation classification: enemy `< 0`, neutral/allied `>= 0` via `UFactionsSubsystem::GetRelation`.

### Factions (`UFactionsSubsystem`)
- `EFaction` enum + fixed flat `int8` relation matrix.
- `GetRelation`/`SetRelation`/`UpdateRelations`/`ResetDefaults` plus `GetNeutralOrAlliedFactions`/`GetEnemyFactions` Blueprint list helpers.
- Defaults: self-relation 0; `VoidRaiders` mutual enemies (`-50`) with all other factions; all other inter-faction relations 0.

### Station Economy (`AStation`)
- `EStationArchetype` enum: `None`, `MiningStation`, `RefineryStation`, `IndustrialStation`, `TradeHub`, `MilitaryOutpost`, `PirateBase`.
- `EGoodsType` enum: `Ore`, `Gas`, `Metals`, `Fuel`, `Parts`, `Food`, `Medicine`, `ConsumerGoods`, `Electronics`, `Ammunition`.
- `StationArchetype`/`SecurityLevel`/`TradeImportance` C++ properties.
- `SupplyGoods` and `DemandGoods` (`TArray<FStationGoodsEntry>`) economy arrays.

### Ship (`AShip`)
- `EnemyShipRadiusBeginOverlap` BlueprintImplementableEvent on enemy `ShipBase` overlap (relation `< 0`).
- `CurrentOpponents` tracking with `AddOpponent`/`RemoveOpponent`/`NotifyIncomingAttack`/`PruneOpponents`.
- Ship presets (`Fighter`/`Interceptor`/`Gunship`/`Cruiser`/`Carrier`) for movement + TorquePD + vitality.
- Vitality presets tune hull/shield/recharge/armor and reset current values to max.
- Blueprint state flags: `isMoving`, `isHalfSpeed`, `IsPlayerLook`.
- `ShipVitalityComponent` with `EDamageType` (`Kinetic`/`Energy`/`Explosive`/`Heat`), shield recharge delay/tick timers.

### External Modules (`AExternalModule`)
- Timer-driven aiming (tick disabled), LOS forward sphere sweep with lead prediction.
- Single/auto/semi-auto fire modes; safe muzzle spawn checks with deferred spawn+finish.
- Effective range from owning ship via `EffectiveRangeMultiplier`.
- Pivot reset to local (0,0,0) when no target; `ReadyToShoot = false` when idle.

### World / Lighting
- `ASun` directional-light splitting with sun visual (mesh root), tracks current view target.
- `ANavStaticBig` asteroid pipeline: spline/signature sphere, near/mid/far HISM streaming with chunk hysteresis, organic jitter/noise/dropout, near-field actor swap for collision/avoidance.
- Circular spline helper defaults to `SignatureSphere` radius × 1.5.
- `ALevelBoundaries` runtime atmosphere system with prediction, non-overlap-by-class, distance-based despawn, and instance cap.

### Level Visual Scheme Pipeline
- `ULevelVisualSchemeData` DataAsset (sun/fog/space-dust/post-process + `SkyboxTexture`).
- `AVagabondsWorldSettings` with per-level `LevelVisualScheme` reference.
- `ULevelVisualSchemeBlueprintLibrary` Blueprint-pure helpers via `WorldContextObject`.

### Player / Input / UI
- `APlayerMainController` with Enhanced Input: throttle (R/F or W/S), pitch/yaw/roll (W/S/A/D/Q/E).
- `APlayerSpectator` with `IA_SpectatorMove`/`IA_SpectatorLook`, smooth look, pitch clamps.
- Possession swap (`Spectator ↔ Ship`) with camera transform snap and boom-length reset.
- `LookAtActor` spectator attach with spring-arm length matching.
- `UMapWidget` with `RefreshMarkers`, player/`NavStaticBig` markers, `ALevelBoundaries` radius scaling.
- `EMarkerType`: `Ship`, `Star`, `Planet`, `Station`, `Debris`, `Component`.

## Pre-2026-05-16 History
- Earlier changes were not tagged in this repository. Versioned entries begin from the `2026-05-16` editor population tag.