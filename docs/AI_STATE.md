# Current Branch / Version
- Branch: `main`
- Latest commit: `0338e19` (pre-visual-scheme baseline)

# Last Completed Task
- Migrated large static obstacle navigation from `AVagabondsWorkGameMode` to `UNavigationSubsystem`: `ANavStaticBig`/child-class obstacle discovery, `SignatureSphere`-based proxy radius/anchors, subsystem ship path queries, and local static avoidance enabled by property.

# Recently Touched Files (last 5–15)
- Source/VagabondsWork/NavigationSubsystem.h (updated)
- Source/VagabondsWork/NavigationSubsystem.cpp (updated)
- Source/VagabondsWork/ShipNavComponent.h (updated)
- Source/VagabondsWork/ShipNavComponent.cpp (updated)
- Source/VagabondsWork/NavStaticBig.cpp (updated)
- Source/VagabondsWork/VagabondsWorkGameMode.h (updated)
- Source/VagabondsWork/VagabondsWorkGameMode.cpp (updated)
- docs/README.md (updated)
- docs/DEVELOPMENT_GUIDE.md (updated)
- docs/CHANGELOG.md (updated)
- docs/AI_STATE.md (updated)
- docs/AI_FILEMAP.md (updated)

# Known Issues / TODO
- If safety margin still re-arms, inspect the new EscapeTarget debug logs for obstacle/normal issues and capture failing obstacle component names.

# Assumptions Confirmed (important!)
- Navigation is timer‑driven; avoid per‑tick heavy traces/pathfinding.
- Steering uses forward thrust + yaw/pitch rotation (roll disabled).
- Static obstacle caching/waypoint planning lives in `UNavigationSubsystem`; `AVagabondsWorkGameMode` stays minimal.
- External modules are tick‑disabled and updated via timers.
- Faction enemies use relation `< 0`; neutral/allied use relation `>= 0`.

# Next Tasks Queue
- Keep AI_STATE updated after future tasks.

# Last Editor Layout Update
- `TestSiteMap` was populated with named planets/stations: Aurelia, Borealis, Cygnus, Dravik, Erebus.
- All planets are placed at separated orbit distances with minimum 18M uu Sun distance.
- Stations are placed on safe orbit offsets around their assigned planets and grouped under `PlanetGroups/<PlanetName>`.
- `BP_UFE` is positioned near Borealis inside expected signature range without planet-body overlap.
