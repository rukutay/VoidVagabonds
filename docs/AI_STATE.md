# Current Branch / Version
- Branch: `Optimization`
- Latest commit: `d7dcd9b` (small fix)

# Last Completed Task
- TASK 15: Added Shoot damage parameter and wired it into projectile damage amount on spawn.

# Recently Touched Files (last 5–15)
- Source/VagabondsWork/ExternalModule.h (updated)
- Source/VagabondsWork/ExternalModule.cpp (updated)
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