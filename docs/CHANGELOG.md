# Changelog

User-facing changes only. Technical/internal details live in `VERSION_CHANGES.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).

## [Unreleased]
### Added
- External ship module system (timer-driven aiming, tick disabled).
- Global navigation replanning with cached static obstacles.

### Improved
- Navigation avoidance stability and replan jittering for scale.
- Ship steering uses physics-driven thrust with yaw/pitch rotation (roll disabled).
- Sun/planet visuals and lighting channel handling (static mesh roots, tick disabled).

### Removed
- Roll aim functionality (removed to keep ship behavior stable).

## [Unknown (needs tag/commit reference)]
- Earlier release history is not tagged in this repo. Add versioned entries once tags/commits are identified.
