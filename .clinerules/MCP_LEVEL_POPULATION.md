ROLE: MCP level-population helper for VagabondsWork Unreal editor sessions.

WHEN POPULATING PLANETS/STATIONS VIA MCP
- Use editor outliner labels as authoritative names; game names follow editor outliner labels.
- `spawn_blueprint_actor` sets internal actor `Name` but may not set the visible outliner label in all MCP paths.
- After spawning, always set `ActorLabel` via `set_actor_property` for planets and stations:
  - `property_name="ActorLabel"`
  - `property_value="<Desired Outliner Name>"`
- Spawn planet BP with full path: `/Game/Assets/BPs/Level/BP_Planet`.
- Spawn station BPs with full paths:
  - `/Game/Assets/BPs/Level/BP_Station`   (inherits AStation — archetype/security/trade)
  - `/Game/Assets/BPs/Level/BP_StationB`  (inherits AStation — archetype/security/trade)
  - `/Game/Assets/BPs/Level/BP_StationA`  (does NOT inherit AStation — avoid for economy stations)
- Set planet `Custom=true` via `set_actor_property`.
- Set planet `CustomPlanetTexture` via `set_actor_property` using `/Game/Assets/PlanetTextures/<Asset>.<Asset>` paths.
- `CustomPlanetSize` may not be visible to the current MCP actor-property setter even though it exists in the BP; set actor scale directly as the practical size control.

BOUNDARY RULES
- Do not guess playable size from actor transform alone.
- Determine placed `BP_LevelBoundaries` and its `Boudaries`/Boundaries sphere component data.
- Current `TestSiteMap` boundary evidence:
  - `BP_LevelBoundaries` actor at `[0,0,0]`, scale `[3,3,3]`.
  - Serialized `Boudaries` `SphereRadius` is `31,250,000`.
  - Effective map radius is about `93,750,000 uu`.
- Place solar-system planets at large radii inside this bound, not near origin.
- Keep vertical displacement within about 9 degrees of the orbital plane.

PLANET/STATION PLACEMENT RULES
- Avoid existing `BP_Planet_C`/`ANavStaticBig` positions visible in MCP actor listing.
- Keep planet actor scale in requested size range when specified, e.g. `18000–25000`.
- Add small non-zero planet rotation for realism, typically 3–16 degrees on pitch/roll while preserving desired yaw/orbital orientation.
- Stations must be named `{PlanetOutlinerName} {RomanNumeral}`.
- Place stations on the parent planet `SignatureSphere` edge, not close to the body.
- If MCP cannot read component radius directly, infer the orbit edge from BP scale convention used in this project: station orbit radius is approximately `100 * PlanetActorScale`.
- Preserve parent/child correlation by moving stations relative to their parent planet center.

ACTOR REPLACEMENT (DELETE + RESPAWN) — CRITICAL
- **Never spawn a new actor with the same internal `Name` as a just-deleted actor.**
- Deleting an actor via MCP may leave the internal name reserved in the level's actor name map until the level is saved/reloaded.
- Spawning with the same name causes a fatal crash: `Cannot generate unique name for '<Name>' in level`.
- **Workaround**: Spawn with a temporary unique internal name (e.g., `PlanetName_RomanNumeral_Temp`), then immediately set `ActorLabel` to the desired outliner name via `set_actor_property`.
  ```
  spawn_blueprint_actor(name="Veloria_II_Temp", ...)
  set_actor_property(name="Veloria_II_Temp", property_name="ActorLabel", property_value="Veloria II")
  ```
- The internal name remains the temporary one; only `ActorLabel` controls what appears in the outliner.

KNOWN MCP LIMITATIONS
- `get_actor_properties` returns only actor-level transform/class/name for these BPs; it does not expose component internals like `SignatureSphere->GetScaledSphereRadius()`.
- `set_actor_property` can set C++/reflected properties like `Custom` and BP object variables like `CustomPlanetTexture`, but may fail for some BP variables (`CustomPlanetSize`) with `Property not found`.
- Use `set_actor_transform` for reliable outliner actor placement and scale changes.
- Use `set_actor_property ActorLabel` for reliable visible outliner naming.