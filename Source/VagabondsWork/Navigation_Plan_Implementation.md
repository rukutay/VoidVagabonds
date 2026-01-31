# UE5 Space Navigation Plan (VoidVagabonds) — Implementation Plan

**Context:** space flight (no navmesh, no gravity), 500+ active ships, physics-driven thrust + rotation.  
**Project files:** `Ship.{h,cpp}`, `AIShipController.{h,cpp}`.  
**Constraint:** do not change core steering/force model; navigation must be cheap and stable (no oscillation, no “stuck hugging obstacle”).

---

## 0) Target behavior (high-level)

Each ship continuously:
1. **Selects a steering target** (goal or avoidance waypoint).
2. **Applies forward thrust** (existing steering force logic) toward that steering target.
3. **Applies rotation** toward that steering target (already in `AAIShipController::ApplyShipRotation`).
4. **Runs cheap obstacle prediction** on a timer (not every frame) and only escalates work when needed.

Avoidance is split into:
- **Dynamic avoidance** (fast, local, reactive): nearby ships/asteroids, etc.
- **Static/physics avoidance** (slower, but still cheap): planets/stations, large rigid bodies.

Stability features:
- **Commit window**: once a side/avoid waypoint is chosen, don’t flip every tick.
- **Stuck detection**: detect low progress/low yaw-rate / near-contact and enter escape mode.
- **Escape sequence**: generate 2–5 waypoints (not one) to “walk out” of contact.

---

## 1) Current code mapping (what already exists)

### 1.1 `AShip` (core loop)
- **Tick**:
  - Runs `ApplyUnstuckForce(DeltaTime)`.
  - Chooses steering target: `Goal` vs `CurrentTargetLocation` when `bHasAvoidanceTarget`.
  - Applies thrust via `ApplySteeringForce` / `ApplyAggressiveSeekForce`.
  - Rotates via `ShipController->ApplyShipRotation(SteeringTarget)`.
- **EvadeThink** (timer-driven):
  - Predicts obstacle collisions with **sphere sweep** (forward probe).
  - Detects stuck signals (distance not closing / yaw stall / low speed while thrusting / near-contact).
  - Runs a **state machine**: `None`, `AvoidCommit`, `StuckEscape`.
  - Chooses avoidance:
    - **Sequence** when overlapping “big obstacle”
    - **A\*** path around static/physics obstacles via `AAIShipController::FindPathAStar3D`
    - **Local grid pick** via `AAIShipController::GetBestGridPointTowardGoal` (used elsewhere in the file)

### 1.2 `AAIShipController` (helpers + pathing)
- `GetFocusLocation()` reads `TargetActor` or player pawn.
- `ApplyShipRotation(TargetLocation)` applies angular velocity to `ShipBase` (physics).
- `GetBestGridPointTowardGoal(...)`:
  - Builds a 3D grid around `GridCenter`
  - Uses cheap overlap test + 1–2 LOS checks
  - Scores by distance-to-goal and alignment
- `FindPathAStar3D(...)`:
  - Coarse 3D A* inside a bounded cube around `GridCenter`
  - Occupancy via overlap, edge validity via LOS

**Conclusion:** the base architecture is already aligned with the “UE5 Space Navigation Plan”. The remaining work is to (a) match the obstacle-group requirements precisely, (b) make prediction more “physical” (speed + mass), and (c) ensure avoidance doesn’t oscillate or get stuck.

---

## 2) Navigation architecture (final)

### 2.1 Modules
**A) Per-ship navigation state (Ship.h / Ship.cpp)**
- Avoidance state machine + timers
- Cached A* path for static/physics obstacles
- Avoidance target smoothing + commit/side-lock
- Debug toggles (draw probe, draw targets, log events)

**B) Controller utilities (AIShipController.h / AIShipController.cpp)**
- Rotation to target
- Cheap local-grid “best point” for dynamic avoidance
- Coarse A* for static/physics avoidance
- Shared collision helpers:
  - `IsPointFree(...)`
  - `HasLineOfSight(...)`

### 2.2 Update rates (cheap at scale)
- `Tick`: steering + rotation only (already done).
- `EvadeThink`: **timer** (already done) with staggered start delay (`FRandRange(0, interval)`).
- Optional scaling:
  - Far ships: larger interval / reduced probe length / disable A*.
  - Near player or combat: smaller interval.

---

## 3) Obstacle groups and collision filters (required)

**Requirement:**
- Group I **StaticObstacles**: `WorldStatic | WorldDynamic`
- Group II **DynamicObstacles**: `Pawn | PhysicsBody`

**Implementation rule:**
- **Forward probe** should detect both groups, but branch logic by channel.
- **Dynamic avoidance logic** must mainly react to `Pawn | PhysicsBody`.
- **Static path logic** must mainly react to `WorldStatic | WorldDynamic` (plus optionally `PhysicsBody` for huge rigid bodies).

### 3.1 Add explicit object query presets
In `Ship.cpp`, define helper builders:
- `BuildObjectQuery_Static()` → WorldStatic + WorldDynamic
- `BuildObjectQuery_Dynamic()` → Pawn + PhysicsBody
- `BuildObjectQuery_AllAvoid()` → union of above

Use these consistently in:
- the forward sweep (AllAvoid)
- local grid occupancy tests (typically AllAvoid, but can be tuned)
- A* occupancy tests (Static only; optionally include PhysicsBody if you treat large asteroids as static for pathing)

---

## 4) Forward prediction (speed + mass, stable, early)

### 4.1 Current probe (already good base)
In `EvadeThink()`, probe distance is:
- min distance based on radius
- plus velocity-based lookahead
- plus padding

### 4.2 Adopt “physical” scaling
Add a mass-aware stopping/turning margin:

- Let:
  - `m = ShipBase->GetMass()`
  - `v = |velocity|`
  - `Fmax = MaxForwardForce * CurrentThrottle` (or your effective thrust)
  - approximate max decel `a ≈ Fmax / max(m,1)`
  - braking distance `d_brake = v^2 / (2a)` (clamped)
  - turning margin `d_turn = v * TurnTime`, where `TurnTime` is derived from `MaxYawSpeed` and an assumed heading change (e.g. 30–60 deg)

Then:
- `DetectDistance = max(MinProbeDistance, v*LookAheadSeconds + Padding, d_brake + Padding, d_turn + Padding)`
- Clamp `DetectDistance` to a sane max (e.g. 30k–80k cm) to avoid pathological cases.

**Why this matters:** heavy ships will start avoiding earlier, reducing “hit then react” behavior.

---

## 5) Avoidance target selection pipeline

### 5.1 State machine (already present; keep it)
- `None`: goal steering
- `AvoidCommit`: normal avoidance with commit/cooldowns
- `StuckEscape`: short escape routine with lateral nudge + optional reverse

### 5.2 Commit window (anti-oscillation)
When you choose a new avoidance side/target:
- store `CommitSideSign` and `CommittedAvoidLocation`
- set `CommitExpireTime = Now + CommitDuration`
- while commit active:
  - prefer targets on the same side unless clearly worse (score hysteresis)
  - only switch sides after `OppositeSideWinCount >= N` (e.g. 2–3 consecutive wins)

This is already partially present; ensure all target updates respect commit.

### 5.3 Escape sequence (2–5 waypoints)
When `bIsStuck` or overlapping big obstacle:
- generate `AvoidanceSequence` of 2–3 points (already does 2). Extend to **3–5** when needed:
  - point1: lateral+forward
  - point2: lateral+forward further
  - point3: forward toward goal direction but offset
  - optional point4/5: only if still overlapping or the first points are not reachable
- navigate through this sequence with acceptance radius `~1.25–1.5 * ShipRadius`.

---

## 6) Strategy selection by obstacle type

When forward probe hits:
1. Identify `ObstacleType` from hit component’s object channel.
2. Determine:
   - `bIsStatic = WorldStatic || WorldDynamic`
   - `bIsDynamic = Pawn || PhysicsBody`

### 6.1 Static obstacles → coarse A*
Use `AAIShipController::FindPathAStar3D`:
- Grid center = hit impact point shifted by hit normal (`+ normal * 0.75R`) (already done)
- Cache the path (`CachedAvoidPath`) and follow waypoints
- Repath only on cooldown (`StaticRepathCooldown` / `AStarRepathCooldown`)
- Waypoint advance when inside `1.25R`

### 6.2 Dynamic obstacles → local grid best-point
Use `AAIShipController::GetBestGridPointTowardGoal`:
- Grid center = hit location (or ship location + forward * small offset)
- NumCells small (5–7)
- Minimal traces:
  - occupancy = overlaps
  - 1–2 LOS checks only (already)
- Apply commit rules to avoid flip-flop.

---

## 7) Debug & tooling (one-frame, cheap)

Keep debug drawing behind flags and `#if !(UE_BUILD_SHIPPING)`:
- Probe line/sphere (already `DebugDrawObstacleProbe`)
- Steering target point (already exists in controller, plus ship-level `DebugDrawAvoidanceTarget`)
- One-frame string near ship: state, speed, stuck timer (already)

Add:
- “Current target location” draw (always one frame) when debug enabled.
- “Current forward check” draw (probe) when enabled.

---

## 8) Performance guidelines (500+ ships)

### 8.1 Traces budget
- Use timer-based `EvadeThink` (already).
- Keep A* bounded:
  - `NumCells` 9–11 (<= 1331 nodes)
  - `MaxExpandedNodes` ~1500 (already)
- Keep LOS traces minimal:
  - local grid: 1–2 LOS checks
  - A*: LOS for neighbor edges is expensive; keep it, but only within small grid bounds.

### 8.2 Ship LOD (recommended)
Add distance-to-player tiers:
- Tier0 (<10k): full avoidance, low interval (e.g. 0.1–0.2s)
- Tier1 (<50k): medium interval (0.25–0.5s)
- Tier2 (far): large interval (0.75–1.5s), skip A* (fallback to tangent)

---

## 9) Concrete work plan (edits to your files)

### Step 1 — Normalize obstacle channels
- In `Ship.cpp`:
  - replace the “one object query” with preset builders and branch static vs dynamic behavior by hit channel.
- In `AIShipController.cpp`:
  - update `IsPointFree(...)` to accept a “mode” (StaticOnly / DynamicOnly / All), or provide two helpers.

### Step 2 — Mass-aware probe distance
- In `Ship.cpp::EvadeThink()`:
  - compute `d_brake` and `d_turn` and incorporate into `DetectDistance`.

### Step 3 — Ensure avoidance target is the same target used for rotation
- In `Ship.cpp::Tick()`:
  - rotation already uses `SteeringTarget`; keep it and ensure `CurrentTargetLocation` is updated only by avoidance logic.
- In `EvadeThink()`:
  - all branches that decide a target must write `CurrentTargetLocation` (not “hit point”).

### Step 4 — Escape sequence 3–5 points
- Extend existing `AvoidanceSequence` generator to produce more points under:
  - overlapping big obstacle, or
  - repeated stuck signals within a short window.

### Step 5 — Commit/hysteresis hardening
- Ensure dynamic target updates obey commit:
  - don’t swap sides every timer tick
  - require consistent “win” count before switching.

### Step 6 — Optional LOD
- Add tiering based on distance-to-player or camera.

---

## 10) Cline (Codex) implementation prompt

Use this prompt in Cline:

```
CONTEXT: UE5 Space Navigation Plan (space, no navmesh). Project uses Ship + AIShipController.
FILES: Source/VagabondsWork/Ship.{h,cpp}, AIShipController.{h,cpp}

GOAL:
Adopt the navigation plan into the existing codebase with minimal disruption.
Keep existing steering and rotation behavior (ApplySteeringForce / ApplyAggressiveSeekForce / ApplyShipRotation) intact.

REQUIREMENTS:
1) Cheap at scale (500+ ships). Avoid heavy per-tick traces. Keep EvadeThink timer-based and staggered.
2) Two obstacle groups:
   - StaticObstacles: WorldStatic | WorldDynamic
   - DynamicObstacles: Pawn | PhysicsBody
   Use correct collision filters and branch avoidance strategy by hit channel.
3) Forward prediction must be earlier and more realistic:
   incorporate ship speed AND mass into probe distance (braking/turning margin).
4) Prevent oscillation:
   add/strengthen commit window + hysteresis so avoidance doesn’t flip left/right every tick.
5) Prevent stuck:
   when stuck/near-contact, generate an avoidance sequence of 3–5 waypoints and follow them with acceptance radius ~1.25–1.5 * ShipRadius.
6) Debug:
   add one-frame debug draw for current forward probe and current steering target (gated by existing debug flags, shipping-safe).

IMPLEMENTATION NOTES:
- Add helper builders for collision object query params (static/dynamic/all).
- Update IsPointFree to support static-only vs dynamic-only occupancy checks (or add new helper).
- Integrate mass-aware probe distance into Ship.cpp::EvadeThink.
- Ensure the steering target used for thrust is the same used for rotation (already uses SteeringTarget; ensure avoidance always updates CurrentTargetLocation, never uses hit point directly).
- Keep A* bounded: NumCells <= 11, MaxExpandedNodes ~1500, repath cooldown.
- Provide final output as full updated contents of modified files (copy-paste ready).

DELIVERABLE:
- Edit code in these files only.
- Output diffs or full-file replacements with clear sections.
```

---

## 11) Acceptance checklist

- [ ] With `ObstacleDebugSettings.bDrawProbe` enabled, probe draws one-frame and detects obstacles before collision at high speed.
- [ ] With heavy ships (higher mass), avoidance triggers earlier than light ships at same speed.
- [ ] When approaching a large station/planet, ship chooses A* waypoints and does not oscillate.
- [ ] When near other ships, ship uses local grid target and maintains a side for at least the commit duration.
- [ ] When stuck hugging an obstacle, ship enters escape state and traverses 3–5 points, then returns to goal.
- [ ] CPU remains stable with 500 ships (EvadeThink interval + stagger works).
