# Station Economy & Archetype Assignment — TestSiteMap

Generated 2026-05-19. Global average inter-station distance: **692.1 km** (in-world scale).

## Summary

| Station | BP Class | Archetype | Sec | Trade | Avg Dist (km) | Scale Factor |
|---------|----------|-----------|-----|-------|---------------|--------------|
| Veloria I | BP_Station_C | IndustrialStation | 3 | 2 | 588.6 | 0.93 |
| Veloria II | BP_StationB_C | TradeHub | 3 | 3 | 580.7 | 0.92 |
| Kharos I | BP_Station_C | MiningStation | 1 | 1 | 574.8 | 0.92 |
| Kharos II | BP_StationB_C | RefineryStation | 1 | 1 | 576.8 | 0.92 |
| Nemorin I | BP_Station_C | MiningStation | 0 | 1 | 613.7 | 0.94 |
| Nemorin II | BP_StationB_C | RefineryStation | 0 | 1 | 610.2 | 0.94 |
| Nemorin III | BP_Station_C | MilitaryOutpost | 4 | 1 | 589.0 | 0.93 |
| Solmara I | BP_StationB_C | IndustrialStation | 0 | 1 | 813.5 | 1.09 |
| Thalren I | BP_Station_C | MiningStation | 0 | 1 | 993.8 | 1.22 |
| Thalren II | BP_StationB_C | PirateBase | 0 | 0 | 980.0 | 1.21 |

## Supply Tables

| Station | Supplies |
|---------|----------|
| Veloria I | Parts:46, Electronics:37, ConsumerGoods:27 |
| Veloria II | ConsumerGoods:27, Food:22, Medicine:22 |
| Kharos I | Ore:73, Gas:27 |
| Kharos II | Metals:55, Fuel:36 |
| Nemorin I | Ore:75, Gas:28 |
| Nemorin II | Metals:56, Fuel:37 |
| Nemorin III | Ammunition:55, Fuel:27 |
| Solmara I | Parts:54, Electronics:43, ConsumerGoods:32 |
| Thalren I | Ore:97, Gas:36 |
| Thalren II | Ammunition:48, ConsumerGoods:48, Fuel:36 |

## Demand Tables

| Station | Demands |
|---------|---------|
| Veloria I | Metals:37, Fuel:27, Food:27 |
| Veloria II | Ore:13, Metals:13, Parts:13, Electronics:13, Fuel:13, Food:13, Medicine:13 |
| Kharos I | Food:36, Medicine:27, Parts:27, Fuel:18 |
| Kharos II | Ore:45, Food:27, Parts:18 |
| Nemorin I | Food:37, Medicine:28, Parts:28, Fuel:18 |
| Nemorin II | Ore:47, Food:28, Parts:18 |
| Nemorin III | Food:37, Medicine:27, Parts:27, Electronics:18 |
| Solmara I | Metals:43, Fuel:32, Food:32 |
| Thalren I | Food:48, Medicine:36, Parts:36, Fuel:24 |
| Thalren II | Medicine:48, Electronics:36, Parts:36 |

## Distance-Based Price Logic

Scale factor formula: `amount = base × (1 + (avg_dist/global_avg − 1) × 0.5)`, clamped to [0.7, 1.5].

- **Central stations** (Veloria, Kharos ~575 km avg): factor ~0.92 — lower supply/demand amounts → more competitive prices, goods are plentiful.
- **Mid-system** (Nemorins ~590–615 km avg): factor ~0.93–0.94 — near baseline.
- **Isolated** (Solmara ~814 km avg): factor 1.09 — goods are scarcer, prices higher.
- **Far-flung** (Thalren ~980–994 km avg): factor 1.21–1.22 — premium prices, high demand for imports, large ore/gas supply reflecting untapped resources.

## Archetype Design Rationale

| Archetype | Count | Reasoning |
|-----------|-------|-----------|
| MiningStation | 3 (Kharos I, Nemorin I, Thalren I) | Distant resource-rich planets; Kharos is nearest mining zone to core |
| RefineryStation | 2 (Kharos II, Nemorin II) | Mid-system for efficient ore transit from mining to industrial |
| IndustrialStation | 2 (Veloria I, Solmara I) | Near trade routes, one per hemisphere |
| TradeHub | 1 (Veloria II) | Central system position with highest TradeImportance |
| MilitaryOutpost | 1 (Nemorin III) | Near Nemorin mining/refining chain for system security |
| PirateBase | 1 (Thalren II) | Far-flung, minimal security, TradeImportance 0 |

## ⚠️ MCP Crash Warning: Same-Name Respawn

**Do NOT spawn a new actor with the same internal `Name` as a just-deleted actor.** UE5's level actor name map may still hold the old name reservation. Doing so causes a fatal crash:

```
Cannot generate unique name for '<Name>' in level
```

**Workaround**: Spawn with a temporary internal name (e.g., `Veloria_II_Temp`), then immediately set `ActorLabel` to the desired outliner name. The outliner shows `ActorLabel`; the internal `Name` remains the temporary one but is invisible to users.

---

## MCP Application Status

- **10× stations**: All `StationArchetype`, `SecurityLevel`, `TradeImportance` applied via MCP ✅
  - 5× `BP_Station_C` (original): Veloria I, Kharos I, Nemorin I, Nemorin III, Thalren I
  - 5× `BP_StationB_C` (replaced from `BP_StationA_C`): Veloria II, Kharos II, Nemorin II, Solmara I, Thalren II
- All replacements spawned with temporary internal names (e.g., `Veloria_II_Temp`) and `ActorLabel` set to the desired outliner name to avoid UE5 name-collision crash.
- **SupplyGoods / DemandGoods** (`TArray<FStationGoodsEntry>`): MCP `set_actor_property` cannot set complex TArray-of-structs. Apply manually in editor using this table.

## Application Instructions (Editor)

For each station in the **Outliner**:

1. Select the station actor.
2. In Details panel, verify **Station | Identity** properties are set (already applied via MCP).
3. Under **Station | Economy**:
   - Populate `SupplyGoods` array entries with `GoodsType` and `Amount` from the supply table.
   - Populate `DemandGoods` array entries with `GoodsType` and `Amount` from the demand table.
