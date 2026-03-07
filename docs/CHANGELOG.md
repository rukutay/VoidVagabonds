# Changelog

User-facing changes only. Technical/internal details live in `VERSION_CHANGES.md`.

Related docs: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).

## [Unreleased]
- Feature details were merged into project docs for this release prep.
- See: [README.md](README.md), [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md), [VERSION_CHANGES.md](VERSION_CHANGES.md).
- Ship navigation avoidance now ignores the current `TargetActor` (including attached parent/child actor relationships) in all AI action modes, preventing target-chase steering from fighting obstacle avoidance.
- AI fight flow now persists across opponent chains: when the current fight target is destroyed, the controller automatically switches to the next closest valid opponent from `CurrentOpponents`.
- AI fight flow now resumes the pre-fight task (`Patroling` / `Moving` / `Following`) only after all opponents are cleared, with fallback state caching to avoid unintended `Idle` transitions.

## [Unknown (needs tag/commit reference)]
- Earlier release history is not tagged in this repo. Add versioned entries once tags/commits are identified.
