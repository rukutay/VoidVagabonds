#include "Visual/LevelVisualSchemeBlueprintLibrary.h"

#include "Visual/VagabondsWorldSettings.h"
#include "Engine/World.h"

const ULevelVisualSchemeData* ULevelVisualSchemeBlueprintLibrary::GetLevelVisualScheme(const UObject* WorldContextObject)
{
	const AVagabondsWorldSettings* WorldSettings = GetVagabondsWorldSettings(WorldContextObject);
	return WorldSettings ? WorldSettings->LevelVisualScheme.Get() : nullptr;
}

AVagabondsWorldSettings* ULevelVisualSchemeBlueprintLibrary::GetVagabondsWorldSettings(const UObject* WorldContextObject)
{
	if (!IsValid(WorldContextObject))
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	return Cast<AVagabondsWorldSettings>(World->GetWorldSettings());
}