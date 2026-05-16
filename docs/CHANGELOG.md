# Changelog

User-facing changes only. Technical/internal details live in `VERSION_CHANGES.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).

## [Unreleased]
- Feature details were merged into project docs for this release prep.
- See: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).
- Added level visual scheme pipeline:
  - New `ULevelVisualSchemeData` DataAsset for level visual tuning values.
  - New `AVagabondsWorldSettings` with per-level `LevelVisualScheme` reference.
  - New `ULevelVisualSchemeBlueprintLibrary` helpers for safe Blueprint access to world settings and active scheme.
  - Added `SkyboxTexture` (`UTextureCube`) parameter to visual scheme data assets.
- Ship navigation avoidance now ignores the current `TargetActor` (including attached parent/child actor relationships) in all AI action modes, preventing target-chase steering from fighting obstacle avoidance.
- AI fight flow now persists across opponent chains: when the current fight target is destroyed, the controller automatically switches to the next closest valid opponent from `CurrentOpponents`.
- AI fight flow now resumes the pre-fight task (`Patroling` / `Moving` / `Following`) only after all opponents are cleared, with fallback state caching to avoid unintended `Idle` transitions.
- Level actor planet queries now return actor lists like station queries, and AI patrol route creation now accepts generic actors instead of only `NavStaticBig` actors.
- Added Blueprint faction relation list helpers and owner-relative station/planet relation queries for enemy vs neutral/allied filtering.

## [Unknown (needs tag/commit reference)]
- Earlier release history is not tagged in this repo. Add versioned entries once tags/commits are identified.

## 2026-05-16 - Editor star system population
- Populated `TestSiteMap` with five named `BP_Planet` actors arranged on separated solar orbit shells with minimum 18M uu Sun distance.
- Added matching faction stations on safe planet-orbit offsets and grouped planet/station actors under `PlanetGroups/<PlanetName>` Outliner folders.
- Moved existing `BP_UFE` into Borealis space inside the expected signature area while avoiding planet mesh overlap.
