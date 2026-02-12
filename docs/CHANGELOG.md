# Changelog

User-facing changes only. Technical/internal details live in `VERSION_CHANGES.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).

## [Unreleased]
### Added
- Player-facing marketing document for portfolio presentation (`docs/marketing.md`).
- UMG top-down map widget (UMapWidget) with marker data for player ship + NavStaticBig and a LevelBoundaries-driven map radius.
- LevelBoundaries runtime atmosphere spawning system (BP-configurable classes, predictive placement, overlap-safe class checks, timed cadence, and max-instance cap).
- External ship module system (timer-driven aiming, tick disabled).
- External module firing params (FireRate, SemiAutoShootsNumber, FireMode, ShootDelay) and Shoot API with safe muzzle spawn + semi-auto burst timing + damage override.
- Toggleable debug logs for unstuck checks, steering source/heading, and nav target/avoidance decisions.
- NavStaticBig asteroid field scaffolding (plane radius, spline, HISM, generation config hooks).
- NavStaticBig circular spline build helper with radius override or plane radius scaling.
- NavStaticBig asteroid field generation along spline with seeded offsets and scale variation.
- NavStaticBig view-based asteroid streaming with near/mid/far HISM tiers, per-tier density/budgets, and editor preview caps.
- NavStaticBig organic jitter/probability spawning, hit-to-dynamic-actor conversion, stable cell sampling, and incremental chunk streaming with hysteresis (double-buffer fallback) to avoid blinking.
- NavStaticBig Blueprint helper to replace a HISM instance with a spawned actor using the same transform and mesh.
- NavStaticBig near-field asteroid actor swap (timer-driven with enter/exit hysteresis) for full collision near the player; asteroids that wake physics remain actors and sleeping swaps now feed runtime navigation anchors.
- Global navigation replanning with cached static obstacles.
- Player manual ship controls (analog throttle, WASD pitch/yaw, Q/E roll) with AI handoff on unpossess, manual roll-align toggle, and ManualYawBankScale for yaw-driven banking when roll-align is disabled.
- Ship presets for movement + TorquePD rotation tuning (Fighter/Interceptor/Gunship/Cruiser/Carrier) with Blueprint-selectable values.
- Ship vitality presets now align with ship presets (hull/shield/recharge/armor) and reset current hull/shield to max on preset apply.

### Improved
- AI ships now passively level roll only while moving forward to keep horizontal alignment without fighting steering.
- Navigation avoidance stability and replan jittering for scale.
- Ship navigation now forces replans on stuck/static-blocked states and always honors temp avoidance targets to prevent long stalls.
- Unstuck recovery now reacquires blocking obstacles, enforces a minimum penetration scale for force, and aligns steering to escape targets to reduce stalls.
- NavStaticBig spawns now bias to a sun-aligned plane with a small 3–7 degree tilt to keep asteroid belts cohesive.
- Safety margin avoidance now ignores self overlaps/invalid obstacles, skips tangent escape when no target actor, suppresses safety checks after forced Nav fallback, and guards against invalid escape targets with debug reasons.
- Dynamic ship avoidance now uses repulsion on predicted closest approach with relative-speed prediction to reduce high-speed head-on collisions.
- Dynamic avoidance now includes awakened/dynamic asteroid actors (WorldDynamic) in neighbor queries.
- Local avoidance now considers blocking PhysicsBody components using bounds-derived radii for non-ship obstacles.
- Debug draw now visualizes PhysicsBody local avoidance bounds when bDrawNavPath is enabled.
- Dynamic avoidance prediction now uses correct relative velocity and blocking-component bounds origin for PhysicsBody obstacles.
- Debug draw now marks predicted closest-approach points used for dynamic avoidance (blue).
- Local avoidance now runs a speed/trajectory-based PhysicsBody line trace to seed avoidance before collision.
- Forward PhysicsBody trace distance now has an editor-tunable multiplier.
- Ship steering uses physics-driven thrust with yaw/pitch rotation (roll disabled).
- NavStaticBig chunk streaming now allocates per-chunk instance budgets to avoid starving late spline sections.
- NavStaticBig chunk streaming now rebuilds when streaming config changes so density/limit tweaks propagate.
- NavStaticBig chunk streaming now rebuilds only dirty chunks using band hysteresis to reduce visible blinking.
- NavStaticBig streaming now adds deterministic along-spline jitter to remove grid-aligned asteroid placement.
- NavStaticBig streaming now uses stratified random candidates and radial offsets to further reduce plane-aligned placement.
- NavStaticBig streaming now supports per-candidate dropout with low-frequency modulation for more organic density variation.
- NavStaticBig streaming dropout now reduces effective per-tier budgets to ensure visible thinning.
- NavStaticBig streaming adds frame-roll, distance/radial noise, and micro-clusters for more organic asteroid breakup.
- Sun/planet visuals and lighting channel handling (static mesh roots, tick disabled).
- Sun directional light now aligns to the Sun-to-player direction (light passes through the Sun toward the pawn).
- Sun directional light rotation now updates every frame and forces Movable mobility for runtime updates.
- Sun directional light now resolves tagged light + pawn every frame and applies rotation to the light component.
- Sun light now prefers current ViewTargetActor each tick (falls back to player pawn).
- External module line-of-sight uses a single forward sphere sweep with lead prediction and projectile radius.
- External modules now reset PivotBase/PivotGun to local rotation (0,0,0) when no target is set, and only aim when TargetActor is valid.

### Removed
- Roll aim functionality (removed to keep ship behavior stable).

## [Unknown (needs tag/commit reference)]
- Earlier release history is not tagged in this repo. Add versioned entries once tags/commits are identified.
