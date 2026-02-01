// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExternalModule.generated.h"

/**
 * Base class for all external ship modules.
 * Provides the core component hierarchy for module attachments.
 * Subclasses should implement specific module functionality.
 */
UCLASS(Abstract)
class VAGABONDSWORK_API AExternalModule : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AExternalModule();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when the actor is being removed from the game
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Target storage - accessible to subclasses
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Targeting")
	AActor* TargetActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Targeting")
	FVector TargetLocation;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Targeting")
	bool bUseTargetActor;

	// Aiming configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Aiming")
	float YawSpeedDegPerSec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Aiming")
	float PitchSpeedDegPerSec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Aiming")
	float MinYawDeg;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Aiming")
	float MaxYawDeg;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Aiming")
	float MinPitchDeg;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Aiming")
	float MaxPitchDeg;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Debug")
	bool bDrawAimDebug;

	// Auto-aim timer configuration
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Aiming")
	bool bAutoAimTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module|Aiming")
	float AimUpdateHz;

	// Timer handle for auto-aim updates
	FTimerHandle AutoAimTimerHandle;

	// Private method called by timer
	void OnAimTimer();

	// Helper functions for aiming calculations
	FVector GetAimDirection() const;

public:
	// Core component hierarchy
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Components")
	USceneComponent* ModuleRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Components")
	USceneComponent* BaseYawPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Components")
	UStaticMeshComponent* BaseMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Components")
	USceneComponent* GunPitchPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Components")
	UStaticMeshComponent* GunMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Module|Components")
	USceneComponent* Muzzle;

	// Blueprint API - Generic implementations that subclasses can override
	UFUNCTION(BlueprintCallable, Category = "Module|Targeting")
	virtual void SetTargetActor(AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category = "Module|Targeting")
	virtual void SetTargetLocation(FVector InLocation);

	UFUNCTION(BlueprintCallable, Category = "Module|Targeting")
	virtual void ClearTarget();

	UFUNCTION(BlueprintCallable, Category = "Module|Targeting")
	virtual FVector GetAimLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Module|Targeting")
	virtual FVector GetMuzzleWorldLocation() const;

	UFUNCTION(BlueprintCallable, Category = "Module|Targeting")
	virtual FVector GetMuzzleWorldForward() const;

	UFUNCTION(BlueprintCallable, Category = "Module|Aiming")
	virtual void AimStep(float DeltaTime);
};
