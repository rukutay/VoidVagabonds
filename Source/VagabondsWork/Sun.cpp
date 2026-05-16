// Fill out your copyright notice in the Description page of Project Settings.


#include "Sun.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Components/LightComponentBase.h"
#include "GameFramework/PlayerController.h"
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

float ASun::ComputeSunViewportCoverage01() const
{
	if (!bEnableSunLookDimming || SunMesh == nullptr)
	{
		return 0.0f;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return 0.0f;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (PC == nullptr)
	{
		return 0.0f;
	}

	int32 ViewX = 0;
	int32 ViewY = 0;
	PC->GetViewportSize(ViewX, ViewY);
	if (ViewX <= 0 || ViewY <= 0)
	{
		return 0.0f;
	}

	FVector CamLoc = FVector::ZeroVector;
	FRotator CamRot = FRotator::ZeroRotator;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	const FVector SunCenter = SunMesh->Bounds.Origin;
	const float SunRadius = SunMesh->Bounds.SphereRadius;
	if (SunRadius <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const FVector ToSun = SunCenter - CamLoc;
	if (FVector::DotProduct(CamRot.Vector(), ToSun) <= 0.0f)
	{
		return 0.0f;
	}

	const FRotationMatrix CamMatrix(CamRot);
	const FVector CamRight = CamMatrix.GetUnitAxis(EAxis::Y);
	const FVector CamUp = CamMatrix.GetUnitAxis(EAxis::Z);

	const int32 SampleCount = FMath::Clamp(DimmingSampleCount, 4, 64);
	int32 InsideCount = 0;

	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		float U = 0.0f;
		float V = 0.0f;

		if (SampleIndex > 0)
		{
			const float t = static_cast<float>(SampleIndex) / static_cast<float>(SampleCount - 1);
			const float R = FMath::Sqrt(t);
			const float Theta = t * 2.0f * PI * 2.39996323f;
			U = R * FMath::Cos(Theta);
			V = R * FMath::Sin(Theta);
		}

		const FVector SampleWorld = SunCenter + (CamRight * U + CamUp * V) * SunRadius;
		FVector2D ScreenPos = FVector2D::ZeroVector;
		if (UGameplayStatics::ProjectWorldToScreen(PC, SampleWorld, ScreenPos, false))
		{
			if (ScreenPos.X >= 0.0f && ScreenPos.X <= static_cast<float>(ViewX) &&
				ScreenPos.Y >= 0.0f && ScreenPos.Y <= static_cast<float>(ViewY))
			{
				++InsideCount;
			}
		}
	}

	return static_cast<float>(InsideCount) / static_cast<float>(SampleCount);
}

void ASun::UpdateSunLighting()
{
	const FVector SunLoc = GetActorLocation();
	float EffectiveIntensity = LightIntensity;

	if (bEnableSunLookDimming)
	{
		const float Coverage01 = ComputeSunViewportCoverage01();
		const float CoverageShaped = FMath::Pow(FMath::Clamp(Coverage01, 0.0f, 1.0f), DimmingResponseExponent);
		const float TargetScale = FMath::Lerp(1.0f, FMath::Clamp(MinIntensityScaleWhenCentered, 0.0f, 1.0f), CoverageShaped);
		const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : (1.0f / FMath::Max(UpdateHz, 1.0f));
		CurrentDimmingScale = FMath::FInterpTo(CurrentDimmingScale, TargetScale, DeltaSeconds, DimmingSmoothingSpeed);
	}
	else
	{
		CurrentDimmingScale = 1.0f;
	}

	EffectiveIntensity *= CurrentDimmingScale;

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
		if (FMath::Abs(EffectiveIntensity - LastAppliedIntensity) > KINDA_SMALL_NUMBER)
		{
			LightComponent->SetIntensity(EffectiveIntensity);
			LastAppliedIntensity = EffectiveIntensity;
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

