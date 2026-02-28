# VagabondsWork

## Overview
VagabondsWork is an Unreal Engine space-flight project focused on AI ship navigation and physics-driven steering in zero gravity (no navmesh).

## Current Project Snapshot (VagabondsWork)
- Engine: **UE 5.5** (authoritative version in [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md)).
- Platforms (from config/docs): **Windows (DX12/SM6)**, **Linux (Vulkan/SM6)**.
- Core module: **VagabondsWork** (Runtime).
- Primary systems (ownership map):
  - `UFactionsSubsystem` → faction enum ownership + fixed 6x6 relation matrix (`int8`, flat cache-friendly storage).
	- `UNavigationSubsystem` → static/runtime obstacle cache, global path anchor planning.
	- `UVagabondsGameInstance` → tracked actor lists + actor filtering utilities.
  - `AVagabondsWorkGameMode` → default pawn setup only.
  - `UShipNavComponent` → global replanning + avoidance + stuck recovery.
  - `AShip` → physics thrust/steering.
  - `AAIShipController` → focus/rotation helpers.
  - `AExternalModule` → timer-driven module aiming.
  - `ANavStaticBig`, `ASun` → large body visuals/lighting helpers.

## Features (verified)
- Timer-driven navigation/avoidance with cached static obstacles (no per-tick heavy pathfinding).
- Faction relations in `UFactionsSubsystem`: allocation-free flat matrix with Blueprint `GetRelation` / `SetRelation` / `ResetDefaults` API.
- Forced replans + unstuck recovery for stuck/static-blocked states; temp avoidance targets are honored.
- Safety/local avoidance hardening: self/invalid overlap filtering, `PhysicsBody` handling, and robust closest-approach prediction.
- `ShipNavComponent` ignores the current intent target actor during avoidance checks (prevents false blocking).

- AI controller action state enum: `Idle`, `Moving`, `Following`, `Patroling`, `Fight`.
- AI action helpers in `AAIShipController`: `StartFollowing(AShip*)`, `MoveToTarget(AActor*)`, `ResetAction()`.
- Following mode behavior: disables orbit, follows assigned ship target, and matches target speed when within `EffectiveRange`.
- Move-to-target behavior: enters `Moving`, disables orbit, moves toward target actor, then auto-resets to idle on arrival (`<= EffectiveRange`).
- AI movement gate via `bMovementAllowed` and nearest-neighbor patrol route generation from `NavStaticBig` candidates.
- Patrol flow cleanup: pause clears `TargetActor`; final point exits patrol back to idle.
- Shared yaw-bank tuning (`YawBankScale`) for AI + player; AI default keeps forward-only passive roll leveling.

- Player manual ship controls (analog throttle + pitch/yaw/roll) with AI handoff on unpossess.
- Spectator flow with Enhanced Input, smooth look, possession swap, and `LookAtActor` helper.
- UMG map widget (`UMapWidget`) with player + `NavStaticBig` markers scaled by `LevelBoundaries`.

- External module system: timer-driven aiming (tick disabled), LOS forward sphere sweep with lead prediction, and single/auto/semi-auto firing modes.
- `NavStaticBig` asteroid pipeline: spline generation, near/mid/far HISM streaming, organic jitter/noise/dropout, and near-field actor swap for collision/avoidance.
- Runtime atmosphere spawning via `ALevelBoundaries` with predictive placement and active-instance cap.
- Sun directional light tracks current view target/player direction.
- Ship presets (movement + TorquePD) and matching vitality presets (hull/shield/recharge/armor).

> TODO: Add any additional gameplay feature claims only after verifying them in code/docs with explicit evidence.

## Build / Run (current setup)
See [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md) for setup, build, and packaging steps.

## Map Widget (Usage)
- Create a Blueprint based on `UMapWidget` and design the UI (draw dots based on the `Markers` array).
- Assign the Blueprint to `APlayerMainController.MapWidgetClass` in defaults (or your controller Blueprint).
- Place an `ALevelBoundaries` actor in the level so the map radius can be resolved.

## Spectator Pawn (Setup)
- Create input actions `IA_SpectatorMove` (Vector3) and `IA_SpectatorLook` (Vector2).
- Create mapping context `IMC_Spectator` and bind WASD + Space/Ctrl for movement and mouse delta for look.
- Assign the mapping context and actions on a BP derived from `APlayerMainController` (spectator input is bound on the controller).
- Movement uses the pawn forward/right/up axes; mouse look smoothly rotates with pitch clamps.

## Possession Swap (Spectator ↔ Ship)
- Create input action `IA_SwitchControl` (Bool) and map it in your input mapping context.
- Assign `SwitchControlAction` on `APlayerMainController`.
- Implement `OnSwitchControlRequested` in a controller Blueprint to pick the target pawn and call `SwitchToPawn(TargetPawn)`.
- Switching from ship back to spectator snaps the spectator to the ship camera transform and resets the spectator boom length to 0 for a smoother transition.
- Use `LookAtActor(AActor* LookAt)` to attach the spectator to any actor and match its spring-arm length when present; passing `None` detaches softly using the attached actor’s spring-arm socket transform.

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
- [marketing.md](marketing.md)
