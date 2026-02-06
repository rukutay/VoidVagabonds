# Version Changes

Technical/internal changes only. User-facing summaries live in `CHANGELOG.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [CHANGELOG.md](CHANGELOG.md).

## [Unreleased]
### Navigation / Avoidance
- Timer-driven global replanning with cached static obstacles.
- Static obstacle refresh for slow-moving bodies (tagged `NavStaticMoving`).
- Temp waypoint tracking for static vs dynamic avoidance; force replan on persistent blocking.

### Steering / Physics
- Forward thrust + yaw/pitch rotation model (roll disabled).
- Soft-separation (airbag) overlap response for gentle braking/repulsion.

### External Modules
- Timer-driven module aiming (tick disabled) with relative-rotation math.
- Debug visualization and readiness checks for turret/module aiming.

### World / Lighting
- Sun and NavStaticBig visuals use static mesh roots; lighting channels configured for planet-only lighting.

## [Unknown (needs tag/commit reference)]
- Earlier release history is not tagged in this repo. Add versioned entries once tags/commits are identified.