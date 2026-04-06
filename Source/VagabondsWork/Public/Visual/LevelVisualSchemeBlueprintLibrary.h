#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LevelVisualSchemeBlueprintLibrary.generated.h"

class AVagabondsWorldSettings;
class ULevelVisualSchemeData;

UCLASS()
class VAGABONDSWORK_API ULevelVisualSchemeBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="Visual", meta=(WorldContext="WorldContextObject"))
	static const ULevelVisualSchemeData* GetLevelVisualScheme(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category="Visual", meta=(WorldContext="WorldContextObject"))
	static AVagabondsWorldSettings* GetVagabondsWorldSettings(const UObject* WorldContextObject);
};