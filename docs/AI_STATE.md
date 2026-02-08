# Current Branch / Version
- Branch: `Optimization`
- Latest commit: `d7dcd9b` (small fix)

# Last Completed Task
- TASK 23: Added near/mid/far HISM tiers for NavStaticBig streaming (impostor-ready).

# Recently Touched Files (last 5–15)
- Source/VagabondsWork/NavStaticBig.h (updated)
- Source/VagabondsWork/NavStaticBig.cpp (updated)
- docs/CHANGELOG.md (updated)
- docs/README.md (updated)
- docs/DEVELOPMENT_GUIDE.md (updated)
- docs/AI_STATE.md (updated)

# Known Issues / TODO
- None tracked in this file yet.

# Assumptions Confirmed (important!)
- Navigation is timer‑driven; avoid per‑tick heavy traces/pathfinding.
- Steering uses forward thrust + yaw/pitch rotation (roll disabled).
- Static obstacle caching/waypoint planning lives in `AVagabondsWorkGameMode`.
- External modules are tick‑disabled and updated via timers.

# Next Tasks Queue
- Keep AI_STATE updated after future tasks.