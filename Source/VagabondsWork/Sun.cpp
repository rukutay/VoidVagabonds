// Fill out your copyright notice in the Description page of Project Settings.


#include "Sun.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Components/LightComponentBase.h"

static bool ComputeDir(const FVector& From, const FVector& To, FVector& OutDir)
{
	OutDir = (To - From);
	if (OutDir.IsNearlyZero())
	{
		return false;
	}
	OutDir.Normalize();
	return true;
}

// Sets default values
ASun::ASun()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SunMesh"));
	SetRootComponent(SunMesh);
	SunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SunMesh->SetGenerateOverlapEvents(false);
	SunMesh->SetCastShadow(false);
}

// Called when the game starts or when spawned
void ASun::BeginPlay()
{
	Super::BeginPlay();
	
	// Auto-find SunDirectionalLight by Actor Tag
	if (bAutoFindSunLight && SunDirectionalLight == nullptr)
	{
		for (TActorIterator<ADirectionalLight> It(GetWorld()); It; ++It)
		{
			if (It->ActorHasTag(SunLightTag))
			{
				SunDirectionalLight = *It;
				break;
			}
		}
	}
	
	// If ViewTargetActor == nullptr, set it to UGameplayStatics::GetPlayerPawn(GetWorld(), 0).
	if (ViewTargetActor == nullptr)
	{
		ViewTargetActor = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	}
	
	if (bForceLightChannels)
	{
		if (SunDirectionalLight != nullptr)
		{
			ULightComponent* LightComponent = SunDirectionalLight->GetLightComponent();
			LightComponent->LightingChannels.bChannel0 = true;
			LightComponent->LightingChannels.bChannel1 = false;
			LightComponent->LightingChannels.bChannel2 = false;
		}

		for (const FSunPlanetLightBinding& Binding : PlanetLights)
		{
			if (Binding.PlanetLight == nullptr)
			{
				continue;
			}
			ULightComponent* PlanetLightComponent = Binding.PlanetLight->GetLightComponent();
			PlanetLightComponent->LightingChannels.bChannel0 = false;
			PlanetLightComponent->LightingChannels.bChannel1 = true;
			PlanetLightComponent->LightingChannels.bChannel2 = false;
			//PlanetLightComponent->bAffectVolumetricFog = false;  // must stay commented !
		}
	}
	
	// Start timer: Interval = 1/UpdateHz (clamp UpdateHz >= 1). Timer calls UpdateSunLighting.
	if (UpdateHz < 1.0f)
	{
		UpdateHz = 1.0f;
	}
	
	GetWorld()->GetTimerManager().SetTimer(LightingTimer, this, &ASun::UpdateSunLighting, 1.0f / UpdateHz, true);

	UpdateAllPlanetLights_Force();
}

void ASun::UpdateSunLighting()
{
	const FVector SunLoc = GetActorLocation();

	if (SunDirectionalLight != nullptr && bDriveGameplayLightDirectionFromViewTarget && ViewTargetActor != nullptr)
	{
		const FVector ViewLoc = ViewTargetActor->GetActorLocation();
		FVector LightDir = FVector::ZeroVector;
		if (ComputeDir(SunLoc, ViewLoc, LightDir))
		{
			if (bInvertLightDirection)
			{
				LightDir *= -1;
			}

			bool bApplyRotation = true;
			if (!LastLightDir.IsZero())
			{
				float DotProduct = FVector::DotProduct(LastLightDir, LightDir);
				DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
				float AngleRad = FMath::Acos(DotProduct);
				float AngleDeg = FMath::RadiansToDegrees(AngleRad);
				bApplyRotation = AngleDeg >= MinAngleDeltaDeg;
			}

			if (bApplyRotation)
			{
				SunDirectionalLight->SetActorRotation(LightDir.Rotation());
				LastLightDir = LightDir;
			}
		}
	}
	
	// Apply light settings if bDriveLightSettings is true
	if (SunDirectionalLight != nullptr && bDriveLightSettings)
	{
		ULightComponent* LightComponent = SunDirectionalLight->GetLightComponent();
		
		// If abs(LightIntensity - LastAppliedIntensity) > KINDA_SMALL_NUMBER: call SetIntensity
		if (FMath::Abs(LightIntensity - LastAppliedIntensity) > KINDA_SMALL_NUMBER)
		{
			LightComponent->SetIntensity(LightIntensity);
			LastAppliedIntensity = LightIntensity;
		}
		
		// If bUseTemperature != LastAppliedUseTemp OR abs(LightTemperature-LastAppliedTemp) > KINDA_SMALL_NUMBER:
		//     set bUseTemperature and temperature on light component
		if (bUseTemperature != LastAppliedUseTemp || FMath::Abs(LightTemperature - LastAppliedTemp) > KINDA_SMALL_NUMBER)
		{
			LightComponent->SetUseTemperature(bUseTemperature);
			if (bUseTemperature)
			{
				LightComponent->SetTemperature(LightTemperature);
			}
			LastAppliedUseTemp = bUseTemperature;
			LastAppliedTemp = LightTemperature;
		}
		
		// If !LightColor.Equals(LastAppliedColor): SetLightColor
		if (!LightColor.Equals(LastAppliedColor))
		{
			LightComponent->SetLightColor(LightColor);
			LastAppliedColor = LightColor;
		}
	}
	
	if (PlanetLastDirs.Num() != PlanetLights.Num())
	{
		PlanetLastDirs.SetNum(PlanetLights.Num());
	}

	for (int32 Index = 0; Index < PlanetLights.Num(); ++Index)
	{
		const FSunPlanetLightBinding& Binding = PlanetLights[Index];
		if (Binding.PlanetActor == nullptr || Binding.PlanetLight == nullptr)
		{
			continue;
		}

		if (!Binding.bPlanetMoves)
		{
			continue;
		}

		FVector PlanetDir = FVector::ZeroVector;
		if (!ComputeDir(SunLoc, Binding.PlanetActor->GetActorLocation(), PlanetDir))
		{
			continue;
		}

		bool bApplyRotation = true;
		if (!PlanetLastDirs[Index].IsZero())
		{
			float DotProduct = FVector::DotProduct(PlanetLastDirs[Index], PlanetDir);
			DotProduct = FMath::Clamp(DotProduct, -1.0f, 1.0f);
			float AngleRad = FMath::Acos(DotProduct);
			float AngleDeg = FMath::RadiansToDegrees(AngleRad);
			bApplyRotation = AngleDeg >= MinAngleDeltaDeg;
		}

		if (bApplyRotation)
		{
			Binding.PlanetLight->SetActorRotation(PlanetDir.Rotation());
			PlanetLastDirs[Index] = PlanetDir;
		}
	}
}

void ASun::UpdateAllPlanetLights_Force()
{
	const FVector SunLoc = GetActorLocation();
	PlanetLastDirs.SetNum(PlanetLights.Num());
	for (int32 Index = 0; Index < PlanetLights.Num(); ++Index)
	{
		const FSunPlanetLightBinding& Binding = PlanetLights[Index];
		if (Binding.PlanetActor == nullptr || Binding.PlanetLight == nullptr)
		{
			continue;
		}

		FVector PlanetDir = FVector::ZeroVector;
		if (!ComputeDir(SunLoc, Binding.PlanetActor->GetActorLocation(), PlanetDir))
		{
			continue;
		}

		Binding.PlanetLight->SetActorRotation(PlanetDir.Rotation());
		PlanetLastDirs[Index] = PlanetDir;
	}
}


void ASun::GetGameplayLightSettings(float& OutIntensity, bool& OutUseTemperature, float& OutTemperature, FLinearColor& OutColor) const
{
	OutIntensity = LightIntensity;
	OutUseTemperature = bUseTemperature;
	OutTemperature = LightTemperature;
	OutColor = LightColor;
}

