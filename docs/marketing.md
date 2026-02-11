# VagabondsWork: Zero-G Fleet Survival

VagabondsWork is a space-flight prototype where you pilot a physics-driven ship through dense asteroid fields while AI squadrons navigate and fight alongside you. The experience is all about momentum, precision steering, and surviving in a living zero-gravity battlefield where every maneuver matters.

## Core Experience
- **Zero-gravity dogfighting.** Manual thrust and pitch/yaw steering make every turn feel physical and earned.
- **Fleet-scale encounters.** AI ships coordinate movement and avoidance without heavy per-tick pathfinding.
- **Asteroid field pressure.** Streaming asteroid belts force you to react to evolving hazards instead of static set dressing.

## Standout Features
- **Physics-first ship handling.** Thrust and rotation are applied directly to the ship’s body for a weighty, inertia-driven feel.
- **Timer-driven navigation.** Global replanning, avoidance, and unstuck recovery are staggered on timers to keep large battles performant.
- **Dynamic obstacle awareness.** Avoidance includes ships, awakened asteroids, and blocking PhysicsBody components for more reliable safety margins.
- **Asteroid streaming at scale.** Near/mid/far HISM tiers, chunk hysteresis, and deterministic jitter keep huge fields dense without visible popping.
- **External module combat.** Timed module aiming and firing support single, semi-auto, and auto fire modes with safe muzzle spawning.

## Implementation Highlights (Portfolio Notes)
- **No navmesh dependency.** Navigation uses global anchors with avoidance-driven steering for true zero-gravity flight.
- **Stuck recovery + safety margins.** Replan-on-stall logic and escape steering prevent long AI deadlocks.
- **Performance-focused architecture.** Timer-driven updates replace per-tick heavy queries, targeting hundreds of active ships.
- **Streaming + swap logic.** Asteroid instances swap to physics actors near the player while preserving navigation anchors for AI.

## Current Status
VagabondsWork is an active prototype focused on building a scalable, physics-led combat playground. The systems above represent verified in-engine features and serve as the foundation for future gameplay expansion.