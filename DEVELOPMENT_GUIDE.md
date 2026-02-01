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
- Built-in targeting system with support for both actor and location targets

### Core Component Hierarchy
- **ModuleRoot**: Root scene component (SetRootComponent)
- **BaseYawPivot**: Yaw rotation pivot attached to ModuleRoot
- **BaseMesh**: Static mesh for base attached to BaseYawPivot
- **GunPitchPivot**: Pitch rotation pivot attached to BaseYawPivot
- **GunMesh**: Static mesh for gun attached to GunPitchPivot
- **Muzzle**: Scene component for projectile spawn point attached to GunPitchPivot

### Targeting System
- **AActor* TargetActor**: Target actor reference with automatic validation
- **FVector TargetLocation**: Manual target location with fallback support
- **bool bUseTargetActor**: Flag to switch between actor and location targeting
- **Automatic target switching**: Seamless transition between actor and location targets
- **Blueprint API**: SetTargetActor(), SetTargetLocation(), ClearTarget(), GetAimLocation()

### Aiming System
- **Configurable speeds**: YawSpeedDegPerSec, PitchSpeedDegPerSec for smooth rotation
- **Angle limits**: Min/Max Yaw and Pitch angles with automatic clamping
- **Local space calculations**: Efficient world-to-local conversion for accurate aiming
- **Smooth interpolation**: RInterpConstantTo for natural rotation behavior
- **Axis isolation**: Pure yaw/pitch rotation with roll disabled for stability

### Auto-Aim Timer System
- **Optional timer-driven updates**: bAutoAimTick flag to enable/disable automatic updates
- **Configurable frequency**: AimUpdateHz property for update rate control (default 20Hz)
- **Performance optimized**: Uses FTimerHandle with proper cleanup in EndPlay()
- **Manual override**: BlueprintCallable AimStep() for custom update control
- **Timer management**: Automatic setup in BeginPlay() and cleanup in EndPlay()

### Debug System
- **Visual debugging**: bDrawAimDebug flag for target visualization
- **Debug drawing**: Red line from muzzle to target, green sphere at target location
- **Real-time feedback**: Immediate visualization of aiming behavior and target positions

### Configuration Variables
- **Aiming Parameters**: All editable via Blueprint with sensible defaults
- **Performance Settings**: Timer frequency and debug toggles for runtime control
- **Category Organization**: Logically grouped properties (Aiming, Debug, Targeting, Components)

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
