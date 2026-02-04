// Fill out your copyright notice in the Description page of Project Settings.


#include "NavStaticBig.h"
#include "Sun.h"
#include "EngineUtils.h"

// Sets default values
ANavStaticBig::ANavStaticBig()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	SetRootComponent(BodyMesh);

	PlanetLight = CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("PlanetLight"));
	PlanetLight->SetupAttachment(BodyMesh);

	BodyMesh->LightingChannels.bChannel0 = false;
	BodyMesh->LightingChannels.bChannel1 = true;
	BodyMesh->LightingChannels.bChannel2 = false;

	PlanetLight->LightingChannels.bChannel0 = false;
	PlanetLight->LightingChannels.bChannel1 = true;
	PlanetLight->LightingChannels.bChannel2 = false;

	//PlanetLight->bAffectVolumetricFog = false;
}

// Called when the game starts or when spawned
void ANavStaticBig::BeginPlay()
{
	Super::BeginPlay();

	ASun* Sun = SunActorOverride;
	if (Sun == nullptr)
	{
		for (TActorIterator<ASun> It(GetWorld()); It; ++It)
		{
			Sun = *It;
			break;
		}
	}

	if (Sun != nullptr && PlanetLight != nullptr)
	{
		ApplySettingsFromSun(Sun);
		if (bDriveRotationFromSun)
		{
			ApplyRotationFromSun(Sun);
		}
	}
}

void ANavStaticBig::ApplySettingsFromSun(const ASun* Sun)
{
	if (Sun == nullptr || PlanetLight == nullptr)
	{
		return;
	}

	float Intensity = 0.0f;
	bool bUseTemperature = false;
	float Temperature = 6500.0f;
	FLinearColor Color = FLinearColor::White;
	Sun->GetGameplayLightSettings(Intensity, bUseTemperature, Temperature, Color);
	PlanetLight->SetIntensity(Intensity);
	PlanetLight->SetLightColor(Color);
	PlanetLight->SetUseTemperature(bUseTemperature);
	if (bUseTemperature)
	{
		PlanetLight->SetTemperature(Temperature);
	}
}

void ANavStaticBig::ApplyRotationFromSun(const ASun* Sun)
{
	if (Sun == nullptr || PlanetLight == nullptr)
	{
		return;
	}

	FVector Dir = (GetActorLocation() - Sun->GetActorLocation());
	if (Dir.IsNearlyZero())
	{
		return;
	}
	Dir.Normalize();
	PlanetLight->SetWorldRotation(Dir.Rotation());
}

