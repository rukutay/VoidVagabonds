# AI File Map — VagabondsWork

## Common Entry Points
- **AVagabondsWorkGameMode** → `Source/VagabondsWork/VagabondsWorkGameMode.h` → `Source/VagabondsWork/VagabondsWorkGameMode.cpp` → GameMode setup, static obstacle caching, global path anchors.
- **AVagabondsWorkCharacter** → `Source/VagabondsWork/VagabondsWorkCharacter.h` → `Source/VagabondsWork/VagabondsWorkCharacter.cpp` → Player character input/camera.
- **AAIShipController** → `Source/VagabondsWork/AIShipController.h` → `Source/VagabondsWork/AIShipController.cpp` → AI focus/rotation + safety margin checks.
- **VagabondsWork module** → `Source/VagabondsWork/VagabondsWork.Build.cs` → `Source/VagabondsWork/VagabondsWork.Build.cs` → build/module config.

## Ship System
- **AShip** → `Source/VagabondsWork/Ship.h` → `Source/VagabondsWork/Ship.cpp` → Physics steering, navigation target handling, soft separation.
- **AShipBase** → `Source/VagabondsWork/AShipBase.h` → `Source/VagabondsWork/AShipBase.cpp` → Base ship actor scaffold.
- **UShipVitalityComponent** → `Source/VagabondsWork/ShipVitalityComponent.h` → `Source/VagabondsWork/ShipVitalityComponent.cpp` → Health/shields/damage application.

## Navigation & Avoidance
- **UShipNavComponent** → `Source/VagabondsWork/ShipNavComponent.h` → `Source/VagabondsWork/ShipNavComponent.cpp` → Global replanning + dynamic/static avoidance.
- **AVagabondsWorkGameMode** → `Source/VagabondsWork/VagabondsWorkGameMode.h` → `Source/VagabondsWork/VagabondsWorkGameMode.cpp` → Static obstacle cache, anchor planning, LOS checks.
- **ANavStaticBig** → `Source/VagabondsWork/NavStaticBig.h` → `Source/VagabondsWork/NavStaticBig.cpp` → Large static bodies used by nav cache.

## Steering & Safety
- **AAIShipController** → `Source/VagabondsWork/AIShipController.h` → `Source/VagabondsWork/AIShipController.cpp` → Rotation control, safety margin, unstuck helpers.

## Weapons / Modules
- **AExternalModule** → `Source/VagabondsWork/ExternalModule.h` → `Source/VagabondsWork/ExternalModule.cpp` → Turret/attachment aiming with timer updates.
- **AProjectile** → `Source/VagabondsWork/Projectile.h` → `Source/VagabondsWork/Projectile.cpp` → Projectile damage + overlap handling.

## Environment / World
- **ASun** → `Source/VagabondsWork/Sun.h` → `Source/VagabondsWork/Sun.cpp` → Sun visuals and lighting control.
- **ALevelBoundaries** → `Source/VagabondsWork/LevelBoundaries.h` → `Source/VagabondsWork/LevelBoundaries.cpp` → World boundary helpers.

## UI / Input
- **AVagabondsWorkCharacter** → `Source/VagabondsWork/VagabondsWorkCharacter.h` → `Source/VagabondsWork/VagabondsWorkCharacter.cpp` → Input bindings, camera boom.