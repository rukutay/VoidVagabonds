// Fill out your copyright notice in the Description page of Project Settings.


#include "Sun.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Components/LightComponentBase.h"
#include "MarkerComponent.h"

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
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickInterval = 0.0f;

	SunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SunMesh"));
	SetRootComponent(SunMesh);
	SunMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SunMesh->SetGenerateOverlapEvents(false);
	SunMesh->SetCastShadow(false);

	MarkerComponent = CreateDefaultSubobject<UMarkerComponent>(TEXT("MarkerComponent"));
	MarkerComponent->MarkerType = EMarkerType::Star;
}

void ASun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateSunLighting();
}

// Called when the game starts or when spawned
void ASun::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(true);
	
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
	
	if (SunDirectionalLight != nullptr)
	{
		ULightComponent* LightComponent = SunDirectionalLight->GetLightComponent();
		if (LightComponent != nullptr && LightComponent->Mobility != EComponentMobility::Movable)
		{
			LightComponent->SetMobility(EComponentMobility::Movable);
		}
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

	}
	
	// Start timer: Interval = 1/UpdateHz (clamp UpdateHz >= 1). Timer calls UpdateSunLighting.
	if (UpdateHz < 1.0f)
	{
		UpdateHz = 1.0f;
	}
	
	if (!PrimaryActorTick.bCanEverTick)
	{
		GetWorld()->GetTimerManager().SetTimer(LightingTimer, this, &ASun::UpdateSunLighting, 1.0f / UpdateHz, true);
	}

	//UpdateAllPlanetLights_Force();
}

void ASun::UpdateSunLighting()
{
	const FVector SunLoc = GetActorLocation();

	if (SunDirectionalLight != nullptr && bDriveGameplayLightDirectionFromViewTarget)
	{
		if (!SunDirectionalLight->IsValidLowLevel())
		{
			SunDirectionalLight = nullptr;
		}
		if (SunDirectionalLight == nullptr && bAutoFindSunLight)
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

		if (SunDirectionalLight == nullptr)
		{
			return;
		}

		AActor* CurrentViewTarget = ViewTargetActor;
		if (CurrentViewTarget == nullptr)
		{
			CurrentViewTarget = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
		}

		if (CurrentViewTarget != nullptr)
		{
			const FVector ViewLoc = CurrentViewTarget->GetActorLocation();
			FVector SunToTargetDir = FVector::ZeroVector;
			if (ComputeDir(SunLoc, ViewLoc, SunToTargetDir))
			{
				// Rays should travel from Sun -> Target by default.
				FVector RaysDir = SunToTargetDir;

				if (bInvertLightDirection)
				{
					RaysDir *= -1.0f;
				}

				// UE directional light rays travel along -X axis.
				const FVector LightX = -RaysDir;
				const FRotator NewRot = LightX.Rotation();

				if (ULightComponent* LightComponent = SunDirectionalLight->GetLightComponent())
				{
					LightComponent->SetWorldRotation(NewRot);
				}
				else
				{
					SunDirectionalLight->SetActorRotation(NewRot);
				}

				LastLightDir = RaysDir;
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
	
}


void ASun::GetGameplayLightSettings(float& OutIntensity, bool& OutUseTemperature, float& OutTemperature, FLinearColor& OutColor) const
{
	OutIntensity = LightIntensity;
	OutUseTemperature = bUseTemperature;
	OutTemperature = LightTemperature;
	OutColor = LightColor;
}

