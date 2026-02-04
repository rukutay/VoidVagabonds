// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "NavStaticBig.generated.h"

class ASun;

UCLASS()
class VAGABONDSWORK_API ANavStaticBig : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANavStaticBig();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Planet")
	UStaticMeshComponent* BodyMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Planet")
	UDirectionalLightComponent* PlanetLight = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Planet|Sun")
	ASun* SunActorOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Planet|Sun")
	bool bDriveRotationFromSun = true;

private:
	void ApplySettingsFromSun(const ASun* Sun);
	void ApplyRotationFromSun(const ASun* Sun);
};
