# Version Changes - Nav-and-ExternalModules Branch

## Overview
This branch focuses on enhancing the navigation system and external module functionality for ship AI behaviors. Key improvements include refined external module aiming systems, enhanced navigation planning, and improved ship rotation controls.

## Key Changes

### External Module System Improvements
- **Enhanced Aiming System**: Replaced `UKismetMathLibrary::FindLookAtRotation` with proper vector math using relative rotations for improved accuracy
- **Yaw Calculation**: Enhanced using parent local space conversion and `FMath::Atan2` 
- **Pitch Calculation**: Improved using PivotBase local space with horizontal magnitude validation
- **Flickering Fix**: Resolved issues when modules are attached to moving ships by using relative instead of world rotations
- **Configurable Controls**: Added yaw/pitch speed interpolation and angle clamping
- **Auto-Aim Timer System**: Implemented efficient timer-driven updates (30Hz default, configurable)
- **Manual Aim Control**: Added `bManualAimStep` flag to disable automatic timer and enable manual control
- **Blueprint API**: Enhanced with `SetTargetActor()`, `ClearTarget()`, and `AimStepManual()` functions
- **Debug Visualization**: Added configurable debug drawing system with on-screen text display

### Navigation System Enhancements
- **Global Replanning**: Implemented periodic global replanning with jitter to avoid synchronization spikes across ships
- **Static Obstacle Refresh**: Added low-frequency refresh system for slow-moving static obstacles (planets/suns/stations)
- **Temp Waypoint System**: Enhanced with reason tracking (static vs dynamic) to prevent premature clearing
- **Static Blocking**: Added accumulation tracking with force replan to prevent near-surface deadlocks
- **Change Detection**: Improved replanning with meaningful change detection (ship movement, goal changes)

### Ship Steering and Physics
- **Soft Separation System**: Enhanced "airbag" force system with very soft braking on radius overlap and light push only on ShipBase hit events
- **Blueprint Parameters**: Exposed soft separation parameters for runtime configuration
- **Debug Visualization**: Added debug visualization for soft separation system (yellow boundary sphere + blue force vector)
- **Performance Optimizations**: Implemented periodic cleanup, efficient top-N overlap selection, and memory pre-allocation
- **Torque Control**: Improved ship rotation logic and torque control systems

### Roll Aim Functionality (Reverted)
- **Feature Addition**: Initially implemented roll aim control for ship targeting with configurable modes (UpToTarget, DownToTarget, Disabled)
- **Roll Error Computation**: Added computation based on target direction with PD control for smooth roll alignment
- **Distance Blending**: Implemented distance-based blending and torque limiting
- **Blueprint Integration**: Exposed roll aim parameters in Ship class for blueprint editing
- **Feature Removal**: Complete rollback of all roll aim functionality as it was deemed unnecessary for current ship behavior requirements

## Technical Details
- **Branch Name**: Nav-and-ExternalModules
- **Status**: Active development branch with production-ready external module and navigation improvements
- **Rollback**: Recent complete removal of roll aim functionality to maintain stable ship behavior
- **Performance**: All changes maintain or improve existing performance characteristics
- **Backward Compatibility**: No breaking changes to existing ship or module APIs

## Files Modified
- `Source/VagabondsWork/ExternalModule.h/cpp` - Enhanced external module aiming system
- `Source/VagabondsWork/AIShipController.h/cpp` - Improved ship rotation and torque controls
- `Source/VagabondsWork/Ship.h/cpp` - Various ship behavior enhancements
- `Source/VagabondsWork/ShipNavComponent.h/cpp` - Enhanced navigation planning and obstacle avoidance

## Documentation Updates
- Updated README.md with enhanced external module system documentation
- Enhanced DEVELOPMENT_GUIDE.md with detailed technical implementation notes
- Maintained CHANGELOG.md with comprehensive feature tracking