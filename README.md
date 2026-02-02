# VagabondsWork

## Overview
VagabondsWork is an Unreal Engine project focused on ship AI and movement behaviors in space environments with no navmesh or gravity.

## Navigation Architecture
The navigation system is designed for scalability (500+ active ships) with physics-driven thrust and rotation:

### Static Obstacle System
- `AVagabondsWorkGameMode` caches tagged static obstacles (`NavStaticBig`) as inflated spheres at BeginPlay
- Each cached obstacle stores precomputed anchor points on an inflated shell for future navigation use
- Supports slow-moving "static big" obstacles (planets/suns/stations) that can move or orbit with periodic refresh
- Low-frequency refresh system (default 1.0s interval) only updates obstacles tagged with `NavStaticMoving`
- Fully static obstacles cost nothing after BeginPlay

### Global Path Planning
- `FindGlobalPathAnchors` builds a temporary node list (start/goal + cached anchors), runs A* with on-demand line-of-sight edges
- Returns up to 8 waypoint anchors (goal included) with debug drawing when `bNavDebugDrawStatic` is enabled
- Uses jittered timing for replanning to avoid synchronization spikes across ships
- Automatic replanning when ships move significantly, goals change, or static blocking persists

### Ship Navigation Component
- `UShipNavComponent` asks the game mode for global waypoints on a replan interval and provides the current navigation target for steering
- Layers lightweight neighbor-based avoidance using ship radius-scaled prediction to generate temporary waypoints
- When the goal location matches the ship position, early-returns with current nav target set to ship position
- Tracks progress toward current nav target and triggers stuck recovery actions (forced replans, avoidance boosts, temp waypoint cleanup)

### Advanced Obstacle Avoidance
- Lightweight static obstacle avoidance using cached obstacle spheres with tangent temp waypoints
- Enhanced temp waypoint system with reason tracking (static vs dynamic) to prevent premature clearing
- Static blocked accumulation tracking prevents near-surface deadlocks by forcing replans after 1.0s
- Ships keep static temp waypoints until segments become clear again, preventing "drop waypoint → fly straight into obstacle → stuck" scenarios

## Steering System
- `AShip` steers toward current waypoint from `UShipNavComponent`, which replans from `AAIShipController::GetFocusLocation()`
- `AAIShipController` provides target focus and rotation (no navigation logic)
- Proximity overlap query for nearest blocking component within (ShipRadius + NavSafetyMargin)
- Instance-editable `TargetActor` (AI|Navigation) for per-ship navigation focus
- Controller acquisition retry during `Tick()` for delayed possession handling
- Forward thrust scaled by heading with lateral damping for natural space movement
- Tunable rotation acceleration via pitch/yaw/roll speeds with roll disabled for stable alignment

## Soft Separation (Airbag) System
- Gentle overlap separation system providing soft braking on radius overlap and light push only on ShipBase hit events
- Eliminates slingshot effects while maintaining smooth avoidance behavior
- Tunable via Blueprint-exposed parameters under `Ship|SoftSeparation`:
  - `SoftSeparationMarginCm`: Extra margin beyond ship radius for activation (default: 30cm)
  - `SoftSeparationStrength`: Base force scale (default: 40000)
  - `SoftSeparationMaxForce`: Force magnitude clamp (default: 90000)
  - `SoftSeparationDamping`: Reduces relative velocity into obstacles (default: 2.0)
  - `SoftSeparationResponse`: Force smoothing interpolation speed (default: 6.0)
  - `SoftSeparationMaxActors`: Maximum overlapping components processed per tick (default: 6)
  - `bDebugSoftSeparation`: Enable debug visualization (yellow sphere + blue force vector)
- Uses relative velocity for natural ship-to-ship collision response
- Performance-optimized with periodic cleanup and efficient top-N overlap selection

## External Ship Modules
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

### Improved Aiming System
- Replaced `UKismetMathLibrary::FindLookAtRotation` with proper vector math using relative rotations
- Enhanced yaw calculation using parent local space conversion and `FMath::Atan2` for improved accuracy
- Improved pitch calculation using PivotBase local space with horizontal magnitude validation
- Fixed flickering issues when modules are attached to moving ships by using relative instead of world rotations
- Enhanced debug visualization with PivotBase forward direction arrow and improved safety checks
