# Implementation Plan

[Overview]
Implement a new PlayerSpectator pawn class with a sphere-rooted camera rig and spectator-style movement, and set it as the default pawn with dedicated Enhanced Input actions/context.

The new pawn will mirror UE SpectatorPawn movement by using FloatingPawnMovement with tuned acceleration/deceleration and input bindings for movement and look, while keeping collision disabled on the root sphere for a free-fly camera. It will also add smooth mouse-driven rotation to orbit around the root sphere, keeping the spring arm/camera setup aligned with controller input. The game mode will be updated to use the new pawn as the default player pawn, and a new input mapping context/actions will be referenced from the spectator pawn for runtime binding.

[Types]
Add a new APawn-derived type for spectator control with component and input references.

- `APlayerSpectator` (Source/VagabondsWork/PlayerSpectator.h/.cpp)
  - Components:
    - `USphereComponent* RootSphere` (RootComponent; collision disabled)
    - `USpringArmComponent* CameraBoom` (attached to RootSphere)
    - `UCameraComponent* SpectatorCamera` (attached to CameraBoom)
    - `UFloatingPawnMovement* Movement` (configured like SpectatorPawn)
  - Input references:
    - `UInputMappingContext* SpectatorMappingContext` (EditAnywhere)
    - `UInputAction* MoveAction` (EditAnywhere)
    - `UInputAction* LookAction` (EditAnywhere)
    - optional speed modifier actions if required (e.g., sprint/slow) - confirm with mapping assets
  - Tuning properties:
    - `float MaxSpeed` (EditAnywhere, defaults to spectator-like speed)
    - `float Acceleration` (EditAnywhere)
    - `float Deceleration` (EditAnywhere)
    - `float LookSensitivity` (EditAnywhere)
    - `float PitchClampMin/Max` (EditAnywhere for camera pitch limits)

[Files]
Create a new spectator pawn class and update game mode default pawn binding.

- New files:
  - `Source/VagabondsWork/PlayerSpectator.h` — declaration of the spectator pawn, components, and input handlers.
  - `Source/VagabondsWork/PlayerSpectator.cpp` — component setup, input bindings, movement, and smooth rotation.
- Modified files:
  - `Source/VagabondsWork/VagabondsWorkGameMode.cpp` — set `DefaultPawnClass` to `APlayerSpectator` (or a BP derived from it if requested).
  - `Source/VagabondsWork/VagabondsWorkGameMode.h` — include forward declaration if needed (or rely on include in cpp).
- No file deletions or moves.
- Config updates: none unless you want default input mapping assets referenced in config instead of via BP defaults.

[Functions]
Add spectator input handlers and update game mode constructor.

- New functions (PlayerSpectator):
  - `void Move(const FInputActionValue& Value);` — handle 2D movement input, convert to forward/right/up vectors.
  - `void Look(const FInputActionValue& Value);` — apply smooth yaw/pitch input and clamp pitch.
  - `virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;` — bind Enhanced Input actions.
  - `virtual void NotifyControllerChanged() override;` — add mapping context to local player subsystem.
- Modified functions:
  - `AVagabondsWorkGameMode::AVagabondsWorkGameMode()` — replace default pawn with PlayerSpectator class.
- No function removals.

[Classes]
Introduce a new pawn class and update game mode default pawn choice.

- New class:
  - `APlayerSpectator` (inherits `APawn`), with root sphere, spring arm, camera, floating movement, and Enhanced Input.
- Modified classes:
  - `AVagabondsWorkGameMode` — constructor sets default pawn to `APlayerSpectator` (or its BP if specified).
- No class removals.

[Dependencies]
No new module dependencies required beyond existing Engine + EnhancedInput modules.

[Testing]
Validate in-editor by possessing the new pawn, confirming camera orbit + free movement, and verifying input bindings.

- Manual tests:
  - PIE: verify pawn spawns as default and camera can yaw/pitch smoothly around RootSphere.
  - Movement: WASD (or mapped inputs) moves in camera-relative directions with acceleration/deceleration similar to SpectatorPawn.
  - Pitch clamp: camera pitch stays within expected limits.
  - Collision: RootSphere has no collision responses.

[Implementation Order]
Implement the new pawn class first, then wire it into the game mode and validate input assets in editor.

1. Create `APlayerSpectator` header/cpp with component setup and movement configuration.
2. Add Enhanced Input bindings and mapping context setup on controller change.
3. Update `AVagabondsWorkGameMode` default pawn class to `APlayerSpectator`.
4. Verify input assets in editor and run PIE to confirm behavior.
