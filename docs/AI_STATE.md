# Current Branch / Version
- Branch: `main`
- Latest commit: `0338e19` (pre-visual-scheme baseline)

# Last Completed Task
- Updated `AAIShipController::CreatePatrolRoute` to remove random patrol selection and generate deterministic nearest-neighbor routes through all provided valid actor candidates from the ship location.

# Recently Touched Files (last 5–15)
- Source/VagabondsWork/FactionsSubsystem.h (updated)
- Source/VagabondsWork/FactionsSubsystem.cpp (updated)
- Source/VagabondsWork/LevelActorsSubsystem.h (updated)
- Source/VagabondsWork/LevelActorsSubsystem.cpp (updated)
- docs/README.md (updated)
- docs/DEVELOPMENT_GUIDE.md (updated)
- docs/CHANGELOG.md (updated)
- docs/AI_STATE.md (updated)
- docs/AI_FILEMAP.md (updated)
- Source/VagabondsWork/AIShipController.cpp (updated)

# Known Issues / TODO
- If safety margin still re-arms, inspect the new EscapeTarget debug logs for obstacle/normal issues and capture failing obstacle component names.

# Assumptions Confirmed (important!)
- Navigation is timer‑driven; avoid per‑tick heavy traces/pathfinding.
- Steering uses forward thrust + yaw/pitch rotation (roll disabled).
- Static obstacle caching/waypoint planning lives in `AVagabondsWorkGameMode`.
- External modules are tick‑disabled and updated via timers.
- Faction enemies use relation `< 0`; neutral/allied use relation `>= 0`.

# Next Tasks Queue
- Keep AI_STATE updated after future tasks.

# Last Editor Layout Update
- `TestSiteMap` was populated with named planets/stations: Aurelia, Borealis, Cygnus, Dravik, Erebus.
- All planets are placed at separated orbit distances with minimum 18M uu Sun distance.
- Stations are placed on safe orbit offsets around their assigned planets and grouped under `PlanetGroups/<PlanetName>`.
- `BP_UFE` is positioned near Borealis inside expected signature range without planet-body overlap.
