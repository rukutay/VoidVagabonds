# VagabondsWork

## Overview
VagabondsWork is an Unreal Engine space-flight project focused on AI ship navigation and physics-driven steering in zero gravity (no navmesh).

## Current Project Snapshot (VagabondsWork)
- Engine: **UE 5.5** (authoritative version in [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md)).
- Platforms (from config/docs): **Windows (DX12/SM6)**, **Linux (Vulkan/SM6)**.
- Core module: **VagabondsWork** (Runtime).
- Primary systems (ownership map):
  - `AVagabondsWorkGameMode` → static obstacle cache + global path anchor planning.
  - `UShipNavComponent` → global replanning + avoidance + stuck recovery.
  - `AShip` → physics thrust/steering.
  - `AAIShipController` → focus/rotation helpers.
  - `AExternalModule` → timer-driven module aiming.
  - `ANavStaticBig`, `ASun` → large body visuals/lighting helpers.

## Features (verified)
- Timer-driven navigation and avoidance (no per-tick heavy pathfinding).
- Navigation forces replans on stuck/static-blocked states and honors temp avoidance targets to prevent stalls.
- Dynamic ship avoidance uses repulsion on predicted closest approach with relative-speed prediction for high-speed head-on safety.
- Physics-driven thrust + yaw/pitch rotation for ship steering.
- Static obstacle caching handled by the game mode; nav component requests replans.
- Timer-driven external module aiming (tick disabled).
- External module LOS uses a single forward sphere sweep with lead prediction and projectile radius.
- External module firing supports single/auto/semi-auto modes with safe muzzle spawn and burst timing derived from FireRate, with per-shot damage override and ShootDelay spacing.
- NavStaticBig supports asteroid field configuration scaffolding (spline, plane radius, HISM, generation params).
- NavStaticBig can build circular splines from plane radius scaling or an explicit override.
- NavStaticBig can generate asteroid instances along the spline using seeded offsets and scale ranges.
- NavStaticBig supports view-based asteroid streaming with near/mid/far HISM tiers, stable cell-based sampling, jittered spawn probabilities, instance budgets, editor preview caps, and incremental chunk streaming with hysteresis (double-buffer fallback) to avoid blinking (impostor-friendly). Chunk rebuilds only trigger when a chunk changes band and now use deterministic along-spline jitter to reduce grid alignment.
- NavStaticBig can convert hit near-tier HISM instances into dynamic physics actors on impact.
- Player ship manual control (R/F throttle steps, WASD pitch/yaw, Q/E roll) with AI handoff on unpossess.
- Sun directional light aims from the sun toward the current player pawn (auto-updating target).

> TODO: Add any additional gameplay feature claims only after verifying them in code/docs with explicit evidence.

## Build / Run (current setup)
See [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md) for setup, build, and packaging steps.

## Project Structure (key paths)
- Module rules: `Source/VagabondsWork/VagabondsWork.Build.cs`
- Game target: `Source/VagabondsWork.Target.cs`
- Game mode: `Source/VagabondsWork/VagabondsWorkGameMode.h/.cpp`
- AI controller: `Source/VagabondsWork/AIShipController.h/.cpp`
- Ship navigation: `Source/VagabondsWork/ShipNavComponent.h/.cpp`
- Ship actor: `Source/VagabondsWork/Ship.h/.cpp`
- External modules: `Source/VagabondsWork/ExternalModule.h/.cpp`
- Static obstacles: `Source/VagabondsWork/NavStaticBig.h/.cpp`
- World lighting: `Source/VagabondsWork/Sun.h/.cpp`

## Performance / Scale Assumptions
- Designed to scale to **500+ active ships** with timer-driven navigation and avoidance.
- Avoid per-tick heavy traces/pathfinding; use staggered timers and cached data.
- External modules keep tick disabled and use timers for updates.

## Related Docs
- [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md)
- [CHANGELOG.md](CHANGELOG.md)
- [VERSION_CHANGES.md](VERSION_CHANGES.md)
