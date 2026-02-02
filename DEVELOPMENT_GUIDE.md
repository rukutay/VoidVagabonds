# Development Guide

## Navigation System Architecture

### Static Obstacle Management
- `AVagabondsWorkGameMode` gathers actors tagged `NavStaticBig` on BeginPlay and caches them as inflated spheres
- Inflation uses `DefaultShipRadiusCm` + `NavSafetyMarginCm` to provide a conservative buffer
- Anchor points are precomputed on a shell (`NavAnchorShellMultiplier`) for future nav sampling
- Debug drawing can be toggled with `bNavDebugDrawStatic` to visualize cached spheres/anchors
- `IsSegmentClearOfStaticObstacles` provides a cheap line-of-sight test against the inflated spheres
- `DebugTestSegmentAgainstStaticObstacles` can draw the tested segment (green = clear, red = blocked)
- `FindGlobalPathAnchors` builds a per-query node list from start/goal plus cached anchors, runs A* with on-demand line-of-sight edges, and returns up to 8 waypoints (goal included)
- When the direct segment is clear, the helper returns only the goal; anchor selection is clamped to ~2000 total anchors using a broad corridor prefilter and per-obstacle limits

### Moving Static Obstacle Support
- Enhanced `AVagabondsWorkGameMode` with `Tick()` function for periodic obstacle refresh
- Low-frequency refresh system (default 1.0s interval) updates only obstacles tagged with `NavStaticMoving`
- Fully static obstacles (no `NavStaticMoving` tag) cost nothing after BeginPlay
- Uses golden angle distribution for consistent anchor point generation during refresh
- Refresh timing uses configurable interval with `StaticObstacleRefreshInterval` property

### Ship Navigation Component
- `UShipNavComponent` queries `AVagabondsWorkGameMode::FindGlobalPathAnchors` at replan intervals
- Implements jittered replanning with `ReplanIntervalMin`/`ReplanIntervalMax` to avoid sync spikes
- Meaningful change detection prevents unnecessary replanning:
  - Movement detection using `ReplanMinMovedDistanceMultiplier`
  - Goal change detection using `ReplanMinGoalDeltaMultiplier`
- Tracks current waypoint target and layers neighbor-based avoidance for dynamic obstacles
- Early-returns when goal location matches ship position, treating zeroed nav targets as invalid
- Monitors progress toward active nav target and triggers stuck recovery (replan, avoidance boost, temp waypoint cleanup)

### Advanced Avoidance Features
- Enhanced temp waypoint system with `ETempWaypointReason` enum (None/Dynamic/Static)
- Reason-based cleanup logic preserves static avoidance waypoints until obstacles are clear
- Static blocked accumulation tracking with `StaticBlockedAccumTime` prevents near-surface deadlocks
- Force replan mechanism triggers after 1.0 second of persistent static blocking
- Neighbor queries use cached intervals (`NeighborQueryInterval`) and capped count (`MaxNeighbors`) for performance
- Dynamic avoidance uses predictive closest-approach tests with ship radius-scaled prediction

## Ship Steering System

### Safety Margin Handling
- Safety margin tracking uses explicit enter/exit margins with hysteresis
- Ships enter when distance < `NavSafetyMargin + ShipRadius`, exit when > (margin + hysteresis multiplier)
- Clears cached nav obstacle references on exit for clean state management
- When inside safety margin, uses `ComputeEscapeTarget` as steering target and skips nav target regeneration
- Timer-driven safety margin checks update `bInsideSafetyMargin` based on closest obstacle contact

### Unstuck Detection and Recovery
- Lightweight stuck detection timer accumulates time when speed below threshold and distance not decreasing
- Toggles `bIsUnstucking` when stuck threshold is reached, applying timer-gated forces at contact points
- Exits unstuck mode when timer expires, speed recovers, or obstacle contact is lost
- Skips nav target regeneration while in unstuck mode to avoid oscillating avoidance targets

### Steering Implementation
- `AAIShipController` provides target focus and rotation (no navigation logic)
- `AShip` exposes instance-editable `TargetActor` (AI|Navigation) for per-ship navigation focus
- Controller acquisition retry during `Tick()` handles delayed possession scenarios
- Forward thrust scaled by heading with lateral damping for natural space movement
- Rotation uses yaw/pitch with roll disabled for stable alignment
- Roll steering is disabled in `ApplyShipRotation` so AI ships align using yaw/pitch only

## Soft Separation (Airbag) System

### System Overview
- Gentle overlap separation system providing soft braking on radius overlap and light push only on ShipBase hit events
- Eliminates slingshot effects while maintaining smooth avoidance behavior
- Forces applied gradually on overlap, gentle push only during actual collision

### Key Components and Tunables
- **SoftSeparationMarginCm**: Extra margin beyond ship radius for activation (default: 30cm)
- **SoftSeparationStrength**: Base force scale (default: 40000)
- **SoftSeparationMaxForce**: Force magnitude clamp (default: 90000)
- **SoftSeparationDamping**: Reduces relative velocity into obstacles (default: 2.0)
- **SoftSeparationResponse**: Force smoothing interpolation speed (default: 6.0)
- **SoftSeparationMaxActors**: Maximum overlapping components processed per tick (default: 6)
- **bEnableSoftSeparation**: Toggle to enable/disable entire system (default: true)
- **bDebugSoftSeparation**: Enable debug visualization (yellow sphere + blue force vector)

### Performance Optimizations
- **Periodic Cleanup**: Invalid overlap entries cleaned up every 60 ticks (≈ 1 second)
- **Efficient Top-N Selection**: O(N*k) insertion sort instead of O(N log N) full sorting
- **Duplicate Avoidance**: Prevents adding same component multiple times to tracking array
- **Memory Pre-allocation**: Reuses arrays with `Reserve()` to avoid dynamic allocations
- **Early Exits**: Quick returns when system disabled or components missing

### Debug Visualization
- **Yellow Sphere**: Activation boundary at (ShipRadius + SoftSeparationMarginCm)
- **Blue Force Vector**: Applied smoothed force vector (scaled for visibility)
- **Red Contact Lines**: Closest points to overlapping components
- **Penetration Text**: Numeric labels showing penetration depth
- All visualization wrapped in `!UE_BUILD_SHIPPING` for performance

### Implementation Details
- Forces applied in `AShip::ApplySoftSeparation()` after steering forces in `Tick()`
- Uses `VInterpTo()` for smooth force interpolation over time
- Implements relative velocity calculation for natural ship-to-ship collision response
- Uses `SmoothStep` interpolation for gentle force ramping based on penetration depth
- Applies forces through `ShipBase->AddForce()` with proper physics integration

## External Ship Module System

### System Overview
- Generic base class `AExternalModule` providing standardized component hierarchy for all external ship attachments
- Abstract base class design allowing easy extension for specific module types (turrets, scanners, etc.)
- Performance-optimized with disabled Actor Tick and efficient timer-based updates
- Built-in targeting system with support for actor targets
- Enhanced Blueprint integration with tooltips and categorized properties

### Core Component Hierarchy
- **ModuleRoot**: Root scene component (SetRootComponent)
- **PivotBase**: Yaw rotation pivot attached to ModuleRoot
- **PivotGun**: Pitch rotation pivot attached to PivotBase
- **MeshBase**: Static mesh for base attached to PivotBase
- **MeshGun**: Static mesh for gun attached to PivotGun
- **Muzzle**: Scene component for projectile spawn point attached to PivotGun

### Targeting System
- **AActor* TargetActor**: Target actor reference that can be set via Blueprint
- **Automatic target validation**: Target is automatically validated each frame
- **Blueprint API**: SetTargetActor(), ClearTarget() for target management

### Aiming System
- **Configurable interpolation speeds**: YawInterpSpeed, PitchInterpSpeed for smooth rotation using FMath::RInterpTo
- **Angle limits**: bLimitPitch flag and MaxPitchAbsDeg for pitch clamping
- **Improved vector math**: Replaced UKismetMathLibrary::FindLookAtRotation with proper vector math using relative rotations
- **Enhanced yaw calculation**: Uses parent local space conversion and FMath::Atan2 for improved accuracy
- **Improved pitch calculation**: Uses PivotBase local space with horizontal magnitude validation
- **Yaw/Pitch separation**: Yaw applied to PivotBase (relative rotation), Pitch to PivotGun (relative rotation)
- **Start point selection**: bUseMuzzleAsStart flag to choose between muzzle and gun pivot as aim start point
- **Mathematical precision**: Proper angle normalization and axis-aware calculations
- **Flicker fix**: Uses relative instead of world rotations to prevent flickering when attached to moving ships

### Auto-Aim Timer System
- **Configurable auto-aim**: bAutoAim flag to enable/disable automatic updates
- **Manual aim control**: bManualAimStep flag to disable timer and enable manual control
- **Configurable frequency**: AimUpdateHz property for update rate control (default 30Hz)
- **Performance optimized**: Uses FTimerHandle with proper cleanup in EndPlay()
- **Manual override**: BlueprintCallable AimStepManual(float DeltaSeconds) for custom update control
- **Timer management**: Automatic setup in BeginPlay() and cleanup in EndPlay()

### Debug System
- **Visual debugging**: bDebugAim flag for debug visualization toggle
- **Debug drawing**: Red line from muzzle to target, blue arrow for muzzle forward, green arrow for desired aim direction
- **Configurable duration**: DebugDrawDuration property for debug element lifetime
- **On-screen text**: Real-time display of current yaw/pitch vs desired yaw/pitch values (250ms interval)
- **Performance optimized**: Debug visualization only active when bDebugAim is true

### Configuration Variables
- **Category Organization**: Logically grouped properties with tooltips:
  - **Aim**: TargetActor, bAutoAim, AimUpdateHz, bManualAimStep, bUseMuzzleAsStart
  - **Aim|Speed**: YawInterpSpeed, PitchInterpSpeed
  - **Aim|Limits**: bLimitPitch, MaxPitchAbsDeg
  - **Aim|Debug**: bDebugAim, DebugDrawDuration
- **Blueprint Integration**: All properties editable via Blueprint with descriptive tooltips
- **Runtime Control**: Properties can be modified at runtime for dynamic configuration

### Performance Characteristics
- **Zero Actor Tick**: PrimaryActorTick.bCanEverTick = false for maximum efficiency
- **Timer-based updates**: Optional auto-aim system uses efficient timer callbacks
- **Memory efficient**: Minimal overhead with proper resource management
- **Scalable design**: Base class supports hundreds of modules without performance impact

### Extension Guidelines
- **Virtual functions**: All Blueprint API functions are virtual for easy overriding
- **Protected members**: Target storage and helper functions accessible to subclasses
- **Generic foundation**: Base implementation provides core functionality for all module types
- **Blueprint integration**: Full support for Blueprint inheritance and customization
