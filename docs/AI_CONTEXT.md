# AI Context — VagabondsWork

## Project Overview
- UE5 space flight project focused on AI ship navigation in zero‑gravity (no navmesh, no gravity).
- Scales to 500+ active ships via timer‑driven navigation and avoidance.
- Physics‑driven thrust/rotation with stable, low‑oscillation steering.
- Static obstacle caching and waypoint planning handled centrally in the game mode.

## Architecture Ownership Map
- **AVagabondsWorkGameMode**: Static obstacle caching (`NavStaticBig`), anchor generation, global path anchor planning.
- **UShipNavComponent**: Global replanning, waypoint selection, dynamic/static avoidance, stuck recovery.
- **AShip**: Physics steering forces, soft separation (airbag), applies rotation via controller.
- **AAIShipController**: Focus sourcing, rotation/aim application, helper spatial queries.
- **AExternalModule**: Turret/attachment aiming system with timer‑driven updates.
- **NavStaticBig**: Planet/large body visuals + lighting channel setup (no tick).
- **ASun**: Directional light splitting and sun visuals (mesh root).

## Do Not Break Invariants
- **Performance**
  - Avoid per‑tick heavy traces/pathfinding; keep timers (jittered replans).
  - No dynamic allocations in hot paths; reuse arrays and cache world/transform access.
  - External modules stay tick‑disabled; use timers for update loops.
- **Gameplay/Steering**
  - Preserve forward‑thrust + yaw/pitch rotation model (roll disabled).
  - Navigation target used for thrust must be the same target used for rotation.
  - Static obstacle avoidance must keep stable temp waypoints (no flip‑flop).
  - Safety margins and stuck recovery must remain conservative and stable.

## Coding Conventions
- **Naming**: Unreal prefixes (A/U/F/E), `b` for booleans, `Cm` suffix for distances.
- **Properties**: Use `UPROPERTY` with clear categories (`Ship|Navigation`, `Aim|Speed`, etc.).
- **Logging/Debug**: Wrap debug drawing in `#if !UE_BUILD_SHIPPING`; prefer toggles (e.g., `bDebug...`).
- **Timers**: Use `FTimerHandle` for navigation/aim updates; avoid per‑tick logic unless strictly necessary.
- **No‑Per‑Tick Rule**: Navigation queries and avoidance decisions must be timer‑driven and staggered.

## AI Workflow Rules (Cost Control)
- Prefer user-provided code snippets for the target function/class.
- If file must be edited, open it once and do not re-open.
- Never open unrelated headers “just to understand”.
- If compilation error requires a header change, request that header explicitly.

## Collision/Trace Channels Used
- **Core object channels**: `WorldStatic`, `WorldDynamic`, `Pawn`, `PhysicsBody`.
- **Custom channels**:
  - `Projectile` (ECC_GameTraceChannel1, default overlap)
  - `Ship` (ECC_GameTraceChannel2, default block)
- **Tags used in navigation**: `NavStaticBig`, `NavStaticMoving` (for periodic refresh).

## How to Add Features (Pattern)
- **New tuning properties**: Add to the owning class with `UPROPERTY(EditAnywhere, BlueprintReadWrite)` and a scoped category.
- **Global/shared data**: Store in `AVagabondsWorkGameMode` (e.g., cached obstacles, shared nav data).
- **Navigation changes**:
  - Global planning → GameMode + `UShipNavComponent`.
  - Steering/forces → `AShip` only (keep physics‑driven thrust model).
  - Rotation/aim math → `AAIShipController` helpers.
- **Modules/attachments**: Subclass `AExternalModule` and keep tick disabled; prefer timer updates.
- **Debug features**: Add toggles + one‑frame draw helpers; keep shipping builds clean.

## Current Build Targets / Platform Notes
- **Windows**: DX12, SM6 (DefaultGraphicsRHI_DX12).
- **Linux**: Vulkan SM6.
- **Hardware target**: Desktop, maximum performance.
- **Ray tracing**: Enabled in renderer settings.
- **Default maps**: `/Game/TestSiteMap.TestSiteMap`.