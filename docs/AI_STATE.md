# Current Branch / Version
- Branch: `main`
- Latest commit: `0338e19` (pre-visual-scheme baseline)

# Last Completed Task
- Fixed `NavStaticBig` shortcut tracing while ships are inside `SignatureSphere`/atmosphere volumes: `UNavigationSubsystem` now ignores cached signature components in object trace queries while preserving traces from the ship position to real blocking geometry.

# Recently Touched Files (last 5–15)
- Source/VagabondsWork/NavigationSubsystem.cpp (updated)
- docs/CHANGELOG.md (updated)
- docs/README.md (updated)
- docs/DEVELOPMENT_GUIDE.md (updated)
- docs/AI_STATE.md (updated)

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
- `TestSiteMap` was populated with named planets/stations: Aurelia, Borealis, Cygnus, Dravik, Erebus.
- All planets are placed at separated orbit distances with minimum 18M uu Sun distance.
- Stations are placed on safe orbit offsets around their assigned planets and grouped under `PlanetGroups/<PlanetName>`.
- `BP_UFE` is positioned near Borealis inside expected signature range without planet-body overlap.
