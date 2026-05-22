# AI Context — VagabondsWork

## Project Overview
- UE5 space flight project focused on AI ship navigation in zero‑gravity (no navmesh, no gravity).
- Scales to 500+ active ships via timer‑driven navigation and avoidance.
- Physics‑driven thrust/rotation with stable, low‑oscillation steering.
- Static obstacle caching and waypoint planning handled centrally in `UNavigationSubsystem`.

## Architecture Ownership Map
- **UNavigationSubsystem**: Static/runtime obstacle caching, `ANavStaticBig` class discovery, signature-sphere anchor generation, global path anchor planning.
- **ULevelActorsSubsystem**: Cached station/planet/ship registration, faction-filtered queries, owner-relative enemy/neutral/allied lookups.
- **AVagabondsWorkGameMode**: Minimal GameMode setup (default pawn class).
- **UShipNavComponent**: Global replanning, waypoint selection, dynamic/static avoidance, stuck recovery.
- **AShip**: Physics steering forces, soft separation (airbag), applies rotation via controller.
- **AAIShipController**: Focus sourcing, rotation/aim application, helper spatial queries.
- **AExternalModule**: Turret/attachment aiming system with timer‑driven updates.
- **NavStaticBig**: Planet/large body visuals + lighting channel setup (no tick).
- **ASun**: Directional light splitting and sun visuals (mesh root).

## MCP Level Population Notes
- Use full BP paths for MCP spawning, e.g. `/Game/Assets/BPs/Level/BP_Planet`, `/Game/Assets/BPs/Level/BP_Station`, `/Game/Assets/BPs/Level/BP_StationA`.
- Outliner labels are the authoritative gameplay/editor names for spawned planets and stations; set `ActorLabel` explicitly through MCP after spawning.
- Placed `BP_LevelBoundaries` in `TestSiteMap` uses actor scale `[3,3,3]` and serialized `Boudaries` sphere radius `31,250,000`, giving an effective radius of about `93,750,000 uu`.
- Station orbits should use each planet `SignatureSphere` edge; current MCP cannot read component radius directly, so infer orbit radius from project convention as about `100 * PlanetActorScale` when component radius is unavailable.
- MCP can set `Custom` and `CustomPlanetTexture`; `CustomPlanetSize` may not be available through the current actor-property setter, so set actor scale directly when needed.
- Add small pitch/roll rotations in the 3–16 degree range to spawned planets for more natural orbital presentation.

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
- **Timers**: Use `FTimerHandle` for navigation/aim updates; avoid per‑tick logic unless strictly necessary.,
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
- **Static nav obstacle recognition**: `ANavStaticBig` and child classes; anchors use each actor's `SignatureSphere` scaled world radius.

## How to Add Features (Pattern)
- **New tuning properties**: Add to the owning class with `UPROPERTY(EditAnywhere, BlueprintReadWrite)` and a scoped category.
- **Global/shared navigation data**: Store in `UNavigationSubsystem` (e.g., cached obstacles, shared nav data).
- **Navigation changes**:
  - Global planning → `UNavigationSubsystem` + `UShipNavComponent`.
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