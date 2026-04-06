#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LevelVisualSchemeData.generated.h"

class UTextureCube;

UCLASS(BlueprintType)
class VAGABONDSWORK_API ULevelVisualSchemeData : public UDataAsset
{
	GENERATED_BODY()

public:
	ULevelVisualSchemeData();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Sun")
	FLinearColor SunColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Sun", meta=(ClampMin="0.0"))
	float SunIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Sun", meta=(ClampMin="1000.0", ClampMax="20000.0"))
	float SunTemperature;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Sun")
	bool bUseSunTemperature;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Global Lightning")
	FLinearColor GlobalLightningColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Global Lightning", meta=(ClampMin="0.0"))
	float GlobalLightningIntensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Fog")
	FLinearColor FogColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Fog", meta=(ClampMin="0.0"))
	float FogDensity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Fog", meta=(ClampMin="0.0"))
	float FogHeightFalloff;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Fog")
	FLinearColor FogInscatteringColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Space Dust")
	FLinearColor SpaceDustColorA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Space Dust")
	FLinearColor SpaceDustColorB;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Space Dust", meta=(ClampMin="0.0"))
	float SpaceDustBrightness;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Skybox")
	TObjectPtr<UTextureCube> SkyboxTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Optional Post Process")
	FLinearColor OptionalPostProcessTint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Level Visual|Optional Post Process", meta=(ClampMin="0.0", ClampMax="1.0"))
	float OptionalPostProcessBlendWeight;
};