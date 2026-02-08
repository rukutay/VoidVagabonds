# Changelog

User-facing changes only. Technical/internal details live in `VERSION_CHANGES.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).

## [Unreleased]
### Added
- External ship module system (timer-driven aiming, tick disabled).
- External module firing params (FireRate, SemiAutoShootsNumber, FireMode, ShootDelay) and Shoot API with safe muzzle spawn + semi-auto burst timing + damage override.
- NavStaticBig asteroid field scaffolding (plane radius, spline, HISM, generation config hooks).
- NavStaticBig circular spline build helper with radius override or plane radius scaling.
- NavStaticBig asteroid field generation along spline with seeded offsets and scale variation.
- NavStaticBig view-based asteroid streaming with near/mid/far HISM tiers, per-tier density/budgets, and editor preview caps.
- NavStaticBig organic jitter/probability spawning, hit-to-dynamic-actor conversion, stable cell sampling, and incremental chunk streaming with hysteresis (double-buffer fallback) to avoid blinking.
- Global navigation replanning with cached static obstacles.
- Player manual ship controls (R/F throttle steps, WASD pitch/yaw, Q/E roll) with AI handoff on unpossess.

### Improved
- Navigation avoidance stability and replan jittering for scale.
- Ship navigation now forces replans on stuck/static-blocked states and always honors temp avoidance targets to prevent long stalls.
- Dynamic ship avoidance now uses repulsion on predicted closest approach with relative-speed prediction to reduce high-speed head-on collisions.
- Ship steering uses physics-driven thrust with yaw/pitch rotation (roll disabled).
- NavStaticBig chunk streaming now rebuilds only dirty chunks using band hysteresis to reduce visible blinking.
- NavStaticBig streaming now adds deterministic along-spline jitter to remove grid-aligned asteroid placement.
- Sun/planet visuals and lighting channel handling (static mesh roots, tick disabled).
- Sun directional light now aligns to the Sun-to-player direction (light passes through the Sun toward the pawn).
- Sun directional light rotation now updates every frame and forces Movable mobility for runtime updates.
- Sun directional light now resolves tagged light + pawn every frame and applies rotation to the light component.
- Sun light now prefers current ViewTargetActor each tick (falls back to player pawn).
- External module line-of-sight uses a single forward sphere sweep with lead prediction and projectile radius.

### Removed
- Roll aim functionality (removed to keep ship behavior stable).

## [Unknown (needs tag/commit reference)]
- Earlier release history is not tagged in this repo. Add versioned entries once tags/commits are identified.
