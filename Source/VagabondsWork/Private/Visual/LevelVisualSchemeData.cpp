#include "Visual/LevelVisualSchemeData.h"

ULevelVisualSchemeData::ULevelVisualSchemeData()
	: SunColor(FLinearColor::White)
	, SunIntensity(10000.f)
	, SunTemperature(6500.f)
	, bUseSunTemperature(true)
	, GlobalLightningColor(FLinearColor::White)
	, GlobalLightningIntensity(0.0f)
	, FogColor(FLinearColor(0.08f, 0.1f, 0.14f, 1.0f))
	, FogDensity(0.001f)
	, FogHeightFalloff(0.2f)
	, FogInscatteringColor(FLinearColor(0.16f, 0.2f, 0.28f, 1.0f))
	, SpaceDustColorA(FLinearColor(0.8f, 0.86f, 1.0f, 1.0f))
	, SpaceDustColorB(FLinearColor(1.0f, 0.78f, 0.62f, 1.0f))
	, SpaceDustBrightness(1.0f)
	, SkyboxTexture(nullptr)
	, OptionalPostProcessTint(FLinearColor::White)
	, OptionalPostProcessBlendWeight(0.0f)
{
}