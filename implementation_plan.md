# Implementation Plan

[Overview]
Implement a level-wide, real-time station-to-station trading simulation driven by supply/demand imbalance, using the existing station data model and subsystem architecture.

The current project already contains foundational economy data in `AStation` (`SupplyGoods`, `DemandGoods`, `StationArchetype`, `TradeImportance`) and faction/ownership infrastructure in `UFactionsSubsystem` and `ULevelActorsSubsystem`, but no runtime market logic exists yet. Specifically, there is no periodic economy tick, no price discovery mechanism, no transfer execution between stations, and no persistence of market state beyond static authored arrays.

The implementation should add a dedicated economy subsystem (`UGameInstanceSubsystem`) to remain aligned with existing architectural patterns (timer-driven, centralized, low-overhead systems). This allows us to keep `AStation` as an authored data owner while moving simulation logic into a reusable, testable, runtime service. Per your clarification, trade scope is **station↔station only** and should run in **real-time via timer**.

At a high level, each tick will: discover active stations, normalize station goods into runtime maps, compute local per-good pressure (demand vs supply), derive price, find eligible station pairs, execute bounded transfers from surplus to deficit, and emit summary telemetry for Blueprint/UI debugging. This gives a practical first version of general trade relations based on demand/supply while preserving performance constraints and existing gameplay systems.

[Types]
Add economy runtime structs and policy enums while reusing existing `EStationGoodsType`/`FStationGoodsEntry` as authored input types.

Detailed type definitions and validation:

1. `UENUM(BlueprintType) enum class ETradeLinkPolicy : uint8` (new)
   - `Open` — all stations may trade.
   - `FactionAlliedOrNeutral` — only if faction relation `>= 0`.
   - `SameFactionOnly` — only same faction.
   - Validation/default: default to `FactionAlliedOrNeutral`.

2. `USTRUCT(BlueprintType) struct FStationGoodsRuntime`
   - `EStationGoodsType GoodsType`
   - `int32 SupplyAmount` (clamp >= 0)
   - `int32 DemandAmount` (clamp >= 0)
   - `int32 NetImbalance` (= `DemandAmount - SupplyAmount`)
   - `float LocalPrice` (clamp to min/max price)
   - Relationship: computed from one station’s authored arrays.

3. `USTRUCT(BlueprintType) struct FTradeTransaction`
   - `TWeakObjectPtr<AStation> Seller`
   - `TWeakObjectPtr<AStation> Buyer`
   - `EStationGoodsType GoodsType`
   - `int32 UnitsTransferred` (> 0)
   - `float UnitPrice` (>= 0)
   - `float TotalValue` (= `UnitsTransferred * UnitPrice`)
   - `float DistanceCm` (optional balancing telemetry)

4. `USTRUCT(BlueprintType) struct FStationEconomyRuntimeState`
   - `TWeakObjectPtr<AStation> Station`
   - `float Credits` (clamp >= 0)
   - `TMap<EStationGoodsType, int32> SupplyByGood`
   - `TMap<EStationGoodsType, int32> DemandByGood`
   - `TMap<EStationGoodsType, float> PriceByGood`
   - `float LastTickUnits`
   - `float LastTickValue`

5. `USTRUCT(BlueprintType) struct FEconomyTickSummary`
   - `int32 StationsProcessed`
   - `int32 TradesExecuted`
   - `float TotalUnitsTransferred`
   - `float TotalTradeValue`
   - `float DeltaSeconds`

6. `AStation` data extensions (existing class)
   - `float StationCredits` (default e.g. `10000.f`, clamp >= 0)
   - `float BasePriceMultiplier` (default `1.0f`, clamp `[0.1, 10.0]`)
   - `bool bEconomyEnabled` (default true)
   - Optional tuning:
     - `int32 MaxUnitsPerTradeTickPerGood`

[Files]
Add a dedicated economy subsystem and minimally extend station data to support real-time trade simulation.

Detailed breakdown:

- New files to create:
  - `Source/VagabondsWork/EconomySubsystem.h`
    - New subsystem class, policies, runtime structs, and Blueprint getters.
  - `Source/VagabondsWork/EconomySubsystem.cpp`
    - Tick loop, station ingest, price computation, eligibility checks, transfer execution.

- Existing files to modify:
  - `Source/VagabondsWork/Station.h`
    - Add credits/price/economy-enable properties and optional per-station caps.
  - `Source/VagabondsWork/Station.cpp`
    - Keep marker setup; optionally initialize/validate new economy defaults only.
  - `Source/VagabondsWork/VagabondsWork.Build.cs`
    - Verify dependency list supports added subsystem includes (minimal/no change expected).

- Files to delete or move:
  - None.

- Configuration file updates:
  - None required for initialization (subsystem auto lifecycle).
  - Optional later: add economy tuning defaults to config if needed.

[Functions]
Introduce new subsystem functions for simulation lifecycle and trade clearing; keep existing station constructor behavior intact.

Detailed breakdown:

New functions in `EconomySubsystem`:

1. `virtual void Initialize(FSubsystemCollectionBase& Collection) override;`
2. `virtual void Deinitialize() override;`
3. `void RunEconomyTick();` — timer callback.
4. `void RefreshStationsFromLevelActors();` — pulls station list from `ULevelActorsSubsystem`.
5. `void BuildOrUpdateStationRuntimeState(AStation* Station);`
6. `float ComputeUnitPrice(const AStation* Station, EStationGoodsType GoodsType, int32 Supply, int32 Demand) const;`
7. `bool CanTradeBetweenStations(const AStation* A, const AStation* B) const;`
8. `int32 ComputeTransferAmount(const FStationEconomyRuntimeState& Seller, const FStationEconomyRuntimeState& Buyer, EStationGoodsType GoodsType, float UnitPrice) const;`
9. `void ExecuteTrade(AStation* Seller, AStation* Buyer, EStationGoodsType GoodsType, int32 Units, float UnitPrice);`
10. `UFUNCTION(BlueprintCallable) FEconomyTickSummary GetLastTickSummary() const;`
11. `UFUNCTION(BlueprintCallable) TArray<FTradeTransaction> GetLastTickTransactions() const;`

Modified functions:
- `AStation::AStation()` (`Source/VagabondsWork/Station.cpp`)
  - Keep current marker behavior.
  - Optional: enforce sensible defaults for new economy properties.

Removed functions:
- None.

[Classes]
Add one new subsystem class and extend one existing class with economy state inputs.

Detailed breakdown:

- New classes:
  - `UEconomySubsystem` (`Source/VagabondsWork/EconomySubsystem.h/.cpp`)
    - Inherits `UGameInstanceSubsystem`.
    - Key methods: initialization/deinitialization, economy tick, transfer and pricing methods.
    - Key properties:
      - `float EconomyTickInterval` (e.g. 0.5–2.0 s)
      - `ETradeLinkPolicy TradeLinkPolicy`
      - `float BaseUnitPrice`, `MinUnitPrice`, `MaxUnitPrice`
      - `int32 GlobalMaxUnitsPerTrade`
      - runtime caches and last tick telemetry.

- Modified classes:
  - `AStation` (`Source/VagabondsWork/Station.h/.cpp`)
    - Add economy tuning/properties only, no major behavioral logic.

- Removed classes:
  - None.

[Dependencies]
No third-party dependency changes are needed.

Integration requirements:
- Reuse existing project subsystems:
  - `ULevelActorsSubsystem` for station discovery.
  - `UFactionsSubsystem` for relation-based eligibility.
- Preserve timer-driven architecture and avoid heavy per-tick operations.
- Keep includes narrow and prefer forward declarations to minimize compile impact.

[Testing]
Use deterministic runtime verification with Blueprint-readable telemetry and controlled map scenarios.

Test scope and validation:
- Setup: place 3–6 `AStation` actors with distinct supply/demand profiles.
- Verify on multiple ticks:
  - trade occurs only station↔station,
  - surplus decreases at sellers, demand decreases at buyers,
  - credits update and never drop below zero,
  - no trade for ineligible faction links under selected policy,
  - disabled stations do not participate.
- Edge cases:
  - empty goods arrays,
  - duplicated goods rows in station arrays (runtime aggregation should be stable),
  - destroyed station during active timer,
  - zero/negative authored amounts (sanitized to zero in runtime ingest).
- Non-functional checks:
  - tick interval tuning does not introduce frame hitches,
  - transaction array size stays bounded (configurable cap if needed).

[Implementation Order]
Implement subsystem and simulation core first, then station data integration, then validation and balancing.

1. Create `UEconomySubsystem` skeleton (`.h/.cpp`) with timer lifecycle.
2. Implement station discovery and runtime-state normalization from `AStation` arrays.
3. Implement pricing formula and trade eligibility (including faction policy).
4. Implement transfer execution and state mutation safeguards.
5. Add Blueprint telemetry (`GetLastTickSummary`, `GetLastTickTransactions`).
6. Extend `AStation` with credits/tuning toggles and minimal constructor/default support.
7. Validate in `TestSiteMap` with controlled station scenarios; tune default parameters.
8. After implementation completion and your approval, update docs (`docs/README.md`, `docs/CHANGELOG.md`, optional `docs/DEVELOPMENT_GUIDE.md`).