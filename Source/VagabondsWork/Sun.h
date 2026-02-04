// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DirectionalLight.h"
#include "Kismet/GameplayStatics.h"
#include "Sun.generated.h"

class UStaticMeshComponent;

USTRUCT(BlueprintType)
struct FSunPlanetLightBinding
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* PlanetActor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ADirectionalLight* PlanetLight = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bPlanetMoves = false;
};

UCLASS()
class VAGABONDSWORK_API ASun : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASun();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	ADirectionalLight* SunDirectionalLight = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	AActor* ViewTargetActor = nullptr; // if null, use player pawn

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	float UpdateHz = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	float MinAngleDeltaDeg = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	bool bInvertLightDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	bool bDriveLightSettings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	bool bDriveGameplayLightDirectionFromViewTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	TArray<FSunPlanetLightBinding> PlanetLights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	bool bForceLightChannels = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting", meta=(ClampMin="0.0"))
	float LightIntensity = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	bool bUseTemperature = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting", meta=(ClampMin="1000.0", ClampMax="20000.0"))
	float LightTemperature = 6500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	FLinearColor LightColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	bool bAutoFindSunLight = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sun|Lighting")
	FName SunLightTag = TEXT("SunLight");

	UFUNCTION(BlueprintCallable, Category="Sun|Lighting")
	void GetGameplayLightSettings(float& OutIntensity, bool& OutUseTemperature, float& OutTemperature, FLinearColor& OutColor) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Sun|Visual")
	UStaticMeshComponent* SunMesh = nullptr;

private:
	FTimerHandle LightingTimer;
	FVector LastLightDir = FVector::ZeroVector;
	TArray<FVector> PlanetLastDirs;

	// Private caches for light settings
	float LastAppliedIntensity = -1.f;
	float LastAppliedTemp = -1.f;
	bool  LastAppliedUseTemp = false;
	FLinearColor LastAppliedColor = FLinearColor(-1,-1,-1,1);

	UFUNCTION()
	void UpdateSunLighting();

	UFUNCTION()
	void UpdateAllPlanetLights_Force();
};
