# Implementation Plan

[Overview]
Fix ship tuning propagation so torque-based AI rotation respects ship-level parameters, ship presets apply to all ship component systems, and player possession uses the same tuning values as AI control.

The current ship tuning pipeline defines steering and torque values on `AShip`, but torque PD values are stored on `AAIShipController` defaults and only partially refreshed, and manual control caches values from an AI controller instance during possession. This creates cases where enabling torque PD in AI ignores tuned parameters and where ship preset changes do not fully re-apply to steering, movement, vitality, and manual control data. The plan introduces a single source of truth for ship tuning on `AShip`, ensures that both AI and player control consistently pull from those values, and applies ship presets in one flow that updates movement, rotation, controller torque, and vitality.

The solution keeps existing patterns: tuning values remain `UPROPERTY` values on `AShip`, preset application stays in `AShip::ApplyShipPreset`, and control is timer-driven (no new per-tick heavy work). We will add explicit tuning sync points for AI controller and manual control, and ensure any tuning changes on ship-level propagate whenever a ship is possessed, unpossessed, or has presets applied.

[Types]
Introduce a struct for ship-wide tuning so all systems share one source of truth.

- `USTRUCT(BlueprintType) FShipTuningSnapshot`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MaxPitchSpeed;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MaxYawSpeed;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float PitchAccelSpeed;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float YawAccelSpeed;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MaxRollSpeed;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float RollAccelSpeed;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MaxRollAngle;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") bool bUseTorquePD;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float TorqueKpPitch;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float TorqueKpYaw;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float TorqueKdPitch;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float TorqueKdYaw;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MaxTorquePitch;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MaxTorqueYaw;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MaxTorqueRoll;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float TorqueRollDamping;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float RollAlignKp;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float RollAlignKd;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MaxForwardForce;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float MinThrottle;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float ThrottleInterpSpeed;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|Tuning") float LateralDamping;`

`AShip` will own a `FShipTuningSnapshot` field and keep it synchronized with existing `UPROPERTY` values to minimize risk. This allows explicit copying to AI controller and manual control data.

[Files]
Modify ship and controller sources to synchronize tuning across AI and player control, plus update preset flows.

- Existing files to modify:
  - `Source/VagabondsWork/Ship.h`
    - Add `FShipTuningSnapshot` definition and `GetTuningSnapshot()` / `ApplyTuningSnapshot()` helpers.
    - Add helpers to push tuning into AI controller and manual control caches.
  - `Source/VagabondsWork/Ship.cpp`
    - Implement snapshot creation and application.
    - Ensure `ApplyShipPreset` updates snapshot, controller tuning, manual caches, and vitality.
    - Ensure `PossessedBy` and `UnPossessed` refresh manual tuning from ship data.
    - Ensure `PostEditChangeProperty` updates snapshot + controller tuning for any tuning-related edits.
  - `Source/VagabondsWork/AIShipController.h`
    - Add setter for roll-align gains if needed to avoid reading from ship directly during torque PD path.
    - Confirm torque tuning setters cover all PD parameters and max clamps.
  - `Source/VagabondsWork/AIShipController.cpp`
    - Ensure ApplyShipRotation uses controller values after ship sync, not defaults.
  - `Source/VagabondsWork/ShipVitalityComponent.h/.cpp`
    - Confirm preset-driven vitality stays in sync (no new types). No changes unless snapshot addition needs a helper.

- New files to create:
  - None.

- Files to delete/move:
  - None.

- Configuration updates:
  - None.

[Functions]
Add snapshot and tuning sync helpers for AI and player control.

- New functions in `Source/VagabondsWork/Ship.h/.cpp`:
  - `FShipTuningSnapshot BuildTuningSnapshot() const;`
    - Reads current `AShip` tuning UPROPERTY values.
  - `void ApplyTuningSnapshot(const FShipTuningSnapshot& Snapshot);`
    - Writes snapshot values back to `AShip` tuning fields.
  - `void SyncControllerTuningFromShip();`
    - Wraps `ApplyControllerTuning()` plus any extra roll-align-related sync needed for AI.
  - `void SyncManualTuningFromShip();`
    - Sets `bManualUseTorquePD`, torque PD gains/clamps, dead zone, roll align gains from ship-level fields.

- Modified functions:
  - `AShip::ApplyShipPreset()`
    - After preset values are applied, call `SyncControllerTuningFromShip()` and `SyncManualTuningFromShip()`, then `Vitality->ApplyVitalityPreset(ShipPreset)`.
  - `AShip::ApplyControllerTuning()`
    - Ensure all torque PD values (Kp, Kd, max torque clamps, roll damping, roll align gains) reach the controller.
  - `AShip::BeginPlay()`
    - If preset applied, sync manual + controller tuning after the preset; if not, sync directly from ship fields.
  - `AShip::PossessedBy()` / `AShip::UnPossessed()`
    - Use `SyncManualTuningFromShip()` when entering manual control so player uses ship-level tuning (not stale controller defaults).
  - `AShip::PostEditChangeProperty()`
    - Extend to handle tuning-related properties by re-syncing controller and manual tuning when changes happen in editor.

- Removed functions:
  - None.

[Classes]
Update the ship and controller classes for unified tuning.

- Modified classes:
  - `AShip` (`Source/VagabondsWork/Ship.h/.cpp`)
    - Add tuning snapshot helpers.
    - Ensure preset + tuning changes propagate to AI controller, manual control, and vitality.
  - `AAIShipController` (`Source/VagabondsWork/AIShipController.h/.cpp`)
    - Ensure torque PD fields are fully set from `AShip` and not left at defaults.

- New classes:
  - None.

- Removed classes:
  - None.

[Dependencies]
No new external dependencies; only internal struct additions and UE headers already present.

- Likely include adjustments in `Ship.h` for new struct definitions (no new modules).
- `VagabondsWork.Build.cs` changes: none.

[Testing]
Use PIE with AI and player possession to validate tuning propagation.

- Test requirements:
  - With AI control and `bUseTorquePD=true`, confirm torque Kp/Kd, clamps, and roll damping affect rotation response.
  - Change `ShipPreset` on a placed ship and confirm movement/rotation values change and vitality preset applies.
  - Adjust ship-level tuning values in editor (or at runtime via blueprint) and verify AI controller reflects changes.
  - Possess a ship as player and verify manual rotation uses same torque PD values as AI.
  - Unpossess and verify AI continues to use updated tuning.

[Implementation Order]
Implement tuning snapshot and sync, then apply across preset and possession flows.

1. Add `FShipTuningSnapshot` to `Ship.h` and implement build/apply helpers in `Ship.cpp`.
2. Add `SyncControllerTuningFromShip()` and `SyncManualTuningFromShip()` and integrate them into `BeginPlay`, `ApplyShipPreset`, and possession logic.
3. Ensure `ApplyControllerTuning()` covers all torque PD values and updates controller state properly.
4. Update `PostEditChangeProperty` to re-sync when tuning values change.
5. Validate in PIE with AI + player possession flows.