# Current Branch / Version
- Branch: `main`
- Latest commit: `0338e19` (pre-visual-scheme baseline)

# Last Completed Task
- Populated `TestSiteMap` through MCP with a new randomly named solar-system layout and corrected it to use real `BP_LevelBoundaries` scale and planet `SignatureSphere` orbit distances.

# Recently Touched Files (last 5–15)
- .clinerules/MCP_LEVEL_POPULATION.md (added)
- docs/AI_STATE.md (updated)
- docs/AI_FILEMAP.md (updated)
- docs/AI_CONTEXT.md (updated)
- Content/TestSiteMap.umap / external actor data (updated via MCP/editor)

# Known Issues / TODO
- Verify in-editor that greedy `NavStaticBig` anchor choices do not oscillate for very sparse anchor counts or unusual target positions.

# Assumptions Confirmed (important!)
- Navigation is timer‑driven; avoid per‑tick heavy traces/pathfinding.
- Steering uses forward thrust + yaw/pitch rotation (roll disabled).
- Static obstacle caching/waypoint planning lives in `UNavigationSubsystem`; `AVagabondsWorkGameMode` stays minimal.
- External modules are tick‑disabled and updated via timers.
- Faction enemies use relation `< 0`; neutral/allied use relation `>= 0`.

# Next Tasks Queue
- Keep AI_STATE updated after future tasks.

# Last Editor Layout Update
- `TestSiteMap` was populated via MCP with new randomly named planets: `Veloria`, `Kharos`, `Nemorin`, `Solmara`, `Thalren`.
- New planet positions use placed `BP_LevelBoundaries` data: actor scale `[3,3,3]`, serialized `Boudaries` `SphereRadius` `31,250,000`, effective radius about `93,750,000 uu`.
- New planets are spread across large solar-system radii instead of clustering near origin.
- New stations are named with `{PlanetName} {RomanNumeral}` and positioned around parent planets at inferred `SignatureSphere` edge distance (`~100 * PlanetActorScale`).
- New planets have small non-zero pitch/roll rotations in the 3–16 degree range for visual realism.
- Outliner labels were fixed via MCP `set_actor_property ActorLabel` for the new planets and stations.
- MCP limitation observed: actor property setter can set `Custom` and `CustomPlanetTexture`, but `CustomPlanetSize` was not visible to the current setter; actor scale was used for planet size.
