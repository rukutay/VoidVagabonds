# Implementation Plan

[Overview]
Create a UMG top-down map widget that renders world-space points with the widget center mapped to world (0,0,0).

The map must display a white dot for the player-possessed `AShip`, purple dots for all `ANavStaticBig` actors, and allow future expansion for other ship colors. The map radius is driven by the `ALevelBoundaries` root `USphereComponent::GetScaledSphereRadius()` so world positions map consistently to widget space. Because there are no existing UI widgets, the plan introduces a C++ `UUserWidget` subclass to gather map data (positions/colors) and a Blueprint to style and layout the visuals.

The solution keeps navigation rules in mind: no heavy per-tick logic. The widget will update map data on a timer (or in `NativeTick` with light iteration if acceptable) and expose a data array to Blueprint for rendering. Player controller will own the widget instance and add it to the viewport on BeginPlay.

[Types]
Introduce data structures that represent map markers and optional map configuration.

- `USTRUCT(BlueprintType) FMapMarkerData`
  - `UPROPERTY(BlueprintReadOnly, Category="Map") FVector2D MapPosition;` (normalized to map radius: -1..1 on each axis)
  - `UPROPERTY(BlueprintReadOnly, Category="Map") FLinearColor Color;`
  - `UPROPERTY(BlueprintReadOnly, Category="Map") float SizePx;` (optional per-marker size for Blueprint)
  - `UPROPERTY(BlueprintReadOnly, Category="Map") TWeakObjectPtr<AActor> SourceActor;` (optional for future extensions)

- `USTRUCT(BlueprintType) FMapSettings`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map") float DefaultMarkerSizePx = 6.0f;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map") FLinearColor PlayerColor = FLinearColor::White;`
  - `UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Map") FLinearColor NavStaticBigColor = FLinearColor(0.6f, 0.2f, 0.8f, 1.0f);`

[Files]
Create map widget classes and update controller + build configuration for UMG.

- New files to create:
  - `Source/VagabondsWork/MapWidget.h`
    - Declare `UMapWidget` (C++ `UUserWidget`) plus `FMapMarkerData`, `FMapSettings`.
  - `Source/VagabondsWork/MapWidget.cpp`
    - Implement marker collection, world-to-map projection, and timer-driven updates.

- Existing files to modify:
  - `Source/VagabondsWork/VagabondsWork.Build.cs`
    - Add `UMG`, `Slate`, and `SlateCore` module dependencies.
  - `Source/VagabondsWork/PlayerMainController.h`
    - Add `UPROPERTY(EditAnywhere)` for `TSubclassOf<UMapWidget>` and `UMapWidget* MapWidgetInstance`.
  - `Source/VagabondsWork/PlayerMainController.cpp`
    - Instantiate and add the map widget to viewport in `BeginPlay`.
  - `Source/VagabondsWork/LevelBoundaries.h`
    - Expose a `BlueprintCallable` getter for `GetBoundaryRadius()` to return `Boudaries->GetScaledSphereRadius()` (optional, if direct access is needed).

- Files to delete/move:
  - None.

- Configuration updates:
  - None beyond `Build.cs` module dependencies.

[Functions]
Add map widget collection and projection helpers.

- New functions in `Source/VagabondsWork/MapWidget.h/.cpp`:
  - `UFUNCTION(BlueprintCallable, Category="Map") void RefreshMarkers();`
    - Collect current markers and update `Markers` array.
  - `UFUNCTION(BlueprintPure, Category="Map") float GetMapRadiusCm() const;`
    - Resolve `ALevelBoundaries` actor and return `Boudaries->GetScaledSphereRadius()`.
  - `FVector2D ProjectWorldToMap(const FVector& WorldLocation, float MapRadiusCm) const;`
    - Convert world `X/Y` into normalized map coordinates centered at `(0,0)`.
  - `void CollectPlayerMarker(TArray<FMapMarkerData>& OutMarkers) const;`
    - Use `GetOwningPlayerPawn()` and cast to `AShip` for player marker.
  - `void CollectNavStaticBigMarkers(TArray<FMapMarkerData>& OutMarkers) const;`
    - Iterate `ANavStaticBig` actors and add purple markers.

- Modified functions:
  - `APlayerMainController::BeginPlay()`
    - Create `MapWidgetInstance` with `CreateWidget<UMapWidget>` and add to viewport.

- Removed functions:
  - None.

[Classes]
Introduce a map widget class and update player controller usage.

- New classes:
  - `UMapWidget` (`Source/VagabondsWork/MapWidget.h/.cpp`)
    - Inherits `UUserWidget`.
    - Owns `TArray<FMapMarkerData> Markers` and `FMapSettings Settings`.
    - Timer-driven updates using `FTimerHandle` on `NativeConstruct` and `NativeDestruct`.

- Modified classes:
  - `APlayerMainController` (`Source/VagabondsWork/PlayerMainController.h/.cpp`)
    - Add widget class/property and spawn instance.
  - `ALevelBoundaries` (`Source/VagabondsWork/LevelBoundaries.h/.cpp`)
    - Provide `GetBoundaryRadius()` helper if direct access to sphere radius is needed by widget.

- Removed classes:
  - None.

[Dependencies]
Add UMG-related modules for the new widget.

- `Source/VagabondsWork/VagabondsWork.Build.cs`
  - Add `UMG`, `Slate`, `SlateCore` to `PublicDependencyModuleNames`.

[Testing]
Validate the map widget in PIE.

- Create a Blueprint based on `UMapWidget` and set it in `APlayerMainController` defaults.
- Verify the map center corresponds to world (0,0,0) and points move correctly with world positions.
- Confirm the player ship shows as a white dot.
- Confirm all `ANavStaticBig` actors show as purple dots.
- Adjust `ALevelBoundaries` sphere radius and ensure map scaling updates.

[Implementation Order]
Implement the widget, wire it up, and validate in PIE.

1. Add `UMG`, `Slate`, `SlateCore` to `VagabondsWork.Build.cs`.
2. Create `UMapWidget` and marker structs with refresh/projection helpers.
3. Update `APlayerMainController` to spawn the map widget at BeginPlay.
4. Add `ALevelBoundaries` radius accessor if needed for clean widget access.
5. Build Blueprint styling and validate in PIE.