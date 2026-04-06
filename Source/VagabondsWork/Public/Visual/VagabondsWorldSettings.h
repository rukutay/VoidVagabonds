#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "VagabondsWorldSettings.generated.h"

class ULevelVisualSchemeData;

UCLASS(BlueprintType)
class VAGABONDSWORK_API AVagabondsWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	AVagabondsWorldSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visual")
	TObjectPtr<ULevelVisualSchemeData> LevelVisualScheme;
};