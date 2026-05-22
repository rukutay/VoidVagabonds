# Current Branch / Version
- Branch: `main`
- Latest commit: `7e91ed2` (released baseline — Ship navigation, AI controller, LevelActorsSubsystem, and content updates)

# Last Completed Task
- Added `GetEnemyShipsOfOwner` and `GetNeutralOrAlliedShipsOfOwner` Blueprint-callable functions to `ULevelActorsSubsystem`. Both iterate the cached `Ships` array and filter by faction relation via `UFactionsSubsystem::GetRelation`: enemies `< 0`, neutral/allied `>= 0`. Follows the existing station/planet owner-relative query pattern exactly.

# Recently Touched Files (last 5–15)
- Source/VagabondsWork/LevelActorsSubsystem.h (added GetEnemyShipsOfOwner, GetNeutralOrAlliedShipsOfOwner declarations)
- Source/VagabondsWork/LevelActorsSubsystem.cpp (added matching definitions)
- docs/AI_CONTEXT.md (added ULevelActorsSubsystem to architecture ownership map)
- docs/AI_STATE.md (updated)
- docs/CHANGELOG.md (updated)

# Known Issues
- Verify in-editor that greedy `NavStaticBig` anchor choices do not oscillate for very sparse anchor counts or unusual target positions.

# Assumptions Confirmed (important!)
- Navigation is timer‑driven; avoid per‑tick heavy traces/pathfinding.
- Steering uses forward thrust + yaw/pitch rotation (roll disabled).
- Static obstacle caching/waypoint planning lives in `UNavigationSubsystem`; `AVagabondsWorkGameMode` stays minimal.
- External modules are tick‑disabled and updated via timers.
- Faction enemies use relation `< 0`; neutral/allied use relation `>= 0`.

# Last Editor Layout Update
- `TestSiteMap` was populated via MCP with new randomly named planets: `Veloria`, `Kharos`, `Nemorin`, `Solmara`, `Thalren`.
- New planet positions use placed `BP_LevelBoundaries` data: actor scale `[3,3,3]`, serialized `Boudaries` `SphereRadius` `31,250,000`, effective radius about `93,750,000 uu`.
- New planets are spread across large solar-system radii instead of clustering near origin.
- New stations are named with `{PlanetName} {RomanNumeral}` and positioned around parent planets at inferred `SignatureSphere` edge distance (`~100 * PlanetActorScale`).
- New planets have small non-zero pitch/roll rotations in the 3–16 degree range for visual realism.
- Outliner labels were fixed via MCP `set_actor_property ActorLabel` for the new planets and stations.
- MCP limitation observed: actor property setter can set `Custom` and `CustomPlanetTexture`, but `CustomPlanetSize` was not visible to the current setter; actor scale was used for planet size.
