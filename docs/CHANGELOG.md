# Changelog

User-facing changes only. Technical/internal details live in `VERSION_CHANGES.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).

## [Unreleased]
### Added
- External ship module system (timer-driven aiming, tick disabled).
- Global navigation replanning with cached static obstacles.
- Player manual ship controls (R/F throttle steps, WASD pitch/yaw, Q/E roll) with AI handoff on unpossess.

### Improved
- Navigation avoidance stability and replan jittering for scale.
- Ship steering uses physics-driven thrust with yaw/pitch rotation (roll disabled).
- Sun/planet visuals and lighting channel handling (static mesh roots, tick disabled).
- Sun directional light now aligns to the Sun-to-player direction (light passes through the Sun toward the pawn).
- Sun directional light rotation now updates every frame and forces Movable mobility for runtime updates.
- Sun directional light now resolves tagged light + pawn every frame and applies rotation to the light component.
- Sun light now prefers current ViewTargetActor each tick (falls back to player pawn).

### Removed
- Roll aim functionality (removed to keep ship behavior stable).

## [Unknown (needs tag/commit reference)]
- Earlier release history is not tagged in this repo. Add versioned entries once tags/commits are identified.
