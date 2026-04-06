# Current Branch / Version
- Branch: `main`
- Latest commit: `0338e19` (pre-visual-scheme baseline)

# Last Completed Task
- Added level visual scheme pipeline (custom world settings + Blueprint helpers + skybox cubemap parameter) and synchronized docs.

# Recently Touched Files (last 5–15)
- Source/VagabondsWork/Public/Visual/LevelVisualSchemeData.h (added)
- Source/VagabondsWork/Private/Visual/LevelVisualSchemeData.cpp (added)
- Source/VagabondsWork/Public/Visual/VagabondsWorldSettings.h (added)
- Source/VagabondsWork/Private/Visual/VagabondsWorldSettings.cpp (added)
- Source/VagabondsWork/Public/Visual/LevelVisualSchemeBlueprintLibrary.h (added)
- Source/VagabondsWork/Private/Visual/LevelVisualSchemeBlueprintLibrary.cpp (added)
- docs/README.md (updated)
- docs/DEVELOPMENT_GUIDE.md (updated)
- docs/CHANGELOG.md (updated)
- docs/AI_STATE.md (updated)

# Known Issues / TODO
- If safety margin still re-arms, inspect the new EscapeTarget debug logs for obstacle/normal issues and capture failing obstacle component names.

# Assumptions Confirmed (important!)
- Navigation is timer‑driven; avoid per‑tick heavy traces/pathfinding.
- Steering uses forward thrust + yaw/pitch rotation (roll disabled).
- Static obstacle caching/waypoint planning lives in `AVagabondsWorkGameMode`.
- External modules are tick‑disabled and updated via timers.

# Next Tasks Queue
- Keep AI_STATE updated after future tasks.