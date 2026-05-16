# Implementation Plan

[Overview]
Add a minimal `AStation` actor (derived from `ANavStaticBig`) that stores blueprint-editable supply and demand goods arrays as pure economy data.

This implementation is limited to data representation only: goods taxonomy and per-goods integer values, attached directly to a new Station actor class so designers can edit values in Details/Blueprint defaults. No runtime processing, balancing, simulation, trade behavior, or integration with existing subsystems is included.

The existing codebase does not currently contain a Station class, but already supports station-like categorization via marker type (`EMarkerType::Station`) and generic actor tracking in `ULevelActorsSubsystem`. Creating `AStation : ANavStaticBig` fits current architecture by reusing world actor conventions and avoids broader system changes.

[Types]
Introduce one blueprint-visible goods enum and one blueprint-visible goods entry struct to represent station economy rows.

Detailed type definitions to add in `Source/VagabondsWork/Station.h`:

- `UENUM(BlueprintType) enum class EStationGoodsType : uint8`
  - Values:
    - `Ore`
    - `Gas`
    - `Metals`
    - `Fuel`
    - `Parts`
    - `Food`
    - `Medicine`
    - `ConsumerGoods`
    - `Electronics`
    - `Ammunition`
  - Validation rules: none (pure enum selection).

- `USTRUCT(BlueprintType) struct FStationGoodsEntry`
  - Fields:
    - `GoodsType` (`EStationGoodsType`), `EditAnywhere`, `BlueprintReadWrite`
    - `Amount` (`int32`), `EditAnywhere`, `BlueprintReadWrite`
  - Validation rules: none (explicitly no runtime validation required).
  - Relationship: each struct row represents one supply/demand entry.

[Files]
Add new Station class files and keep all economy storage local to Station for minimal scope.

- New files to create:
  - `Source/VagabondsWork/Station.h`
    - Declares `AStation : public ANavStaticBig`
    - Declares `EStationGoodsType` and `FStationGoodsEntry`
    - Adds `SupplyGoods` and `DemandGoods` arrays
  - `Source/VagabondsWork/Station.cpp`
    - Minimal constructor implementation and include wiring

- Existing files to modify:
  - None required for this scoped data-only feature.

- Files to delete or move:
  - None.

- Configuration updates:
  - None expected (`VagabondsWork.Build.cs` already covers required modules).

[Functions]
Function changes are limited to required constructor scaffolding for a new actor class.

- New functions:
  - `AStation::AStation()`
    - Signature: `AStation();`
    - File: `Source/VagabondsWork/Station.cpp`
    - Purpose: initialize class defaults only if needed (minimal/no extra logic).

- Modified functions:
  - None.

- Removed functions:
  - None.

[Classes]
Add one new class and do not alter existing classes.

- New classes:
  - `AStation`
    - File: `Source/VagabondsWork/Station.h` / `Source/VagabondsWork/Station.cpp`
    - Inheritance: `ANavStaticBig`
    - Key members:
      - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station|Economy") TArray<FStationGoodsEntry> SupplyGoods;`
      - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Station|Economy") TArray<FStationGoodsEntry> DemandGoods;`

- Modified classes:
  - None.

- Removed classes:
  - None.

[Dependencies]
No dependency changes are required.

The implementation uses existing Unreal Engine modules already configured in `Source/VagabondsWork/VagabondsWork.Build.cs`.

[Testing]
Use editor-level validation focused on Details panel/Blueprint editability.

Test requirements:
- Create or open a Blueprint based on `AStation`.
- Confirm both `SupplyGoods` and `DemandGoods` are visible and editable.
- Add array entries and verify each row exposes:
  - `GoodsType` dropdown with all required goods enum values.
  - `Amount` integer field.

No runtime simulation tests are required for this task.

[Implementation Order]
Implement in a minimal, compile-safe order centered on new Station files.

1. Create `Station.h` with enum, struct, and `AStation` class declaration deriving from `ANavStaticBig`.
2. Add `SupplyGoods` and `DemandGoods` properties as `EditAnywhere` + `BlueprintReadWrite` arrays of `FStationGoodsEntry`.
3. Create `Station.cpp` with constructor implementation and required includes.
4. Verify reflection macros/categories compile conceptually (no extra logic/helpers).
5. Validate in editor that arrays and row fields are blueprint-editable per acceptance criteria.