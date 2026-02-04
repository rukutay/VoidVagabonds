# Changelog

## [Unreleased]
### Added
- **External Ship Module System:**
  - Generic base class `AExternalModule` for all external ship attachments with standardized component hierarchy
  - Core component structure: ModuleRoot → PivotBase → MeshBase, PivotGun → MeshGun, Muzzle
  - Built-in targeting system with actor targets and automatic target switching
  - Configurable aiming behavior with yaw/pitch speed interpolation and angle clamping
  - Optional auto-aim timer system for cheap, timer-driven updates (30Hz default, configurable)
  - Manual aim control system with bManualAimStep flag to disable automatic timer
  - Blueprint API for control: SetTargetActor(), ClearTarget(), AimStepManual()
  - BlueprintPure getters for current state: GetMuzzleWorldLocation(), GetMuzzleWorldForward(), GetCurrentYaw(), GetCurrentPitch()
  - Enhanced Blueprint integration with tooltips and categorized properties
  - Debug visualization system with configurable debug drawing toggle and on-screen text
  - Performance-optimized with disabled Actor Tick and efficient timer-based updates
  - Abstract base class design allowing easy extension for specific module types (turrets, scanners, etc.)

### Improved
- **AIShipController Performance:**
  - Cached World/transform access in rotation and safety checks to reduce per-call overhead
  - Switched safety margin comparisons to squared distance tests to avoid sqrt cost
- **ShipNavComponent Avoidance:**
  - Added idle backoff for neighbor overlap queries when ships are slow and no neighbors are found
- **Sun Lighting Split:**
  - Added per-planet directional light bindings driven by sun-to-planet direction
  - Added option to drive gameplay sun light from view target direction
  - Added optional light channel forcing to separate gameplay vs planet lights
- **Sun Visuals:**
  - Removed Sun disk billboard system in favor of a static mesh root
- **NavStaticBig Planet Setup:**
  - Added mesh root and dedicated per-planet directional light component
  - Forced lighting channels to isolate planet lighting
  - Disabled Tick for NavStaticBig actors
  - Added optional runtime sync of planet light settings from Sun (timer-driven)
- **External Ship Module Aiming System:**
  - Replaced `UKismetMathLibrary::FindLookAtRotation` with proper vector math using relative rotations
  - Enhanced yaw calculation using parent local space conversion and `FMath::Atan2` for improved accuracy
  - Improved pitch calculation using PivotBase local space with horizontal magnitude validation
  - Fixed flickering issues when modules are attached to moving ships by using relative instead of world rotations
  - Enhanced debug visualization with PivotBase forward direction arrow and improved safety checks

- **Navigation System Enhancements:**
  - Periodic global replanning with jitter to avoid synchronization spikes across ships
  - Low-frequency refresh system for slow-moving static obstacles (planets/suns/stations)
  - Enhanced temp waypoint system with reason tracking (static vs dynamic) to prevent premature clearing
  - Static blocked accumulation tracking with force replan to prevent near-surface deadlocks
  - Jittered replanning intervals with meaningful change detection (ship movement, goal changes)
- Soft separation "airbag" force system that provides very soft braking on radius overlap and light push only on ShipBase hit events. Force now applies gradual slowdown on overlap and gentle push only when actual collision occurs, eliminating slingshot effects.
- Blueprint-exposed soft separation parameters: SoftSeparationMarginCm, SoftSeparationStrength, SoftSeparationMaxForce, SoftSeparationDamping, SoftSeparationResponse, SoftSeparationMaxActors
- Debug visualization for soft separation system (yellow boundary sphere + blue force vector) via bDebugSoftSeparation toggle
- Relative velocity calculations for ship-to-ship interactions for more natural collision response
- Performance optimizations including periodic cleanup, efficient top-N overlap selection, and memory pre-allocation

### Changed
- Enhanced static obstacle system to support moving obstacles with periodic refresh (tagged `NavStaticMoving`)
- Improved temp waypoint cleanup logic to preserve static avoidance waypoints until obstacles are clear
- Added force replan mechanism when static blocking persists for more than 1.0 second
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

### Removed
- **Roll Aim Functionality**: Complete rollback of ship roll aim control system that was previously implemented for ship targeting. Feature was removed to maintain stable ship behavior and simplify navigation controls.
