// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ExternalModule.generated.h"

class USceneComponent;
class UStaticMeshComponent;

/**
 * External module with auto-aiming capability using yaw/pitch rotation system.
 * Components are arranged as: ModuleRoot -> PivotBase -> MeshBase + PivotGun -> MeshGun + Muzzle
 */
UCLASS()
class VAGABONDSWORK_API AExternalModule : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AExternalModule();

protected:
	// Components
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* ModuleRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* PivotBase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* PivotGun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* MeshBase;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	UStaticMeshComponent* MeshGun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Components")
	USceneComponent* Muzzle;

	// Targeting - Visible and editable in Details panel and BP defaults
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ToolTip = "Target actor for aiming"))
	AActor* TargetActor;

	// Auto-aim settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ToolTip = "Enable automatic aiming updates"))
	bool bAutoAim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ClampMin = "1.0", ClampMax = "120.0", ToolTip = "Aim update frequency in Hz"))
	float AimUpdateHz;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ToolTip = "Enable manual aim stepping instead of automatic timer"))
	bool bManualAimStep;

	// Limits
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Limits", meta = (ToolTip = "Enable pitch angle limits"))
	bool bLimitPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Limits", meta = (ClampMin = "0.0", ClampMax = "89.9", ToolTip = "Maximum absolute pitch angle in degrees"))
	float MaxPitchAbsDeg;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Orbit", meta=(ClampMin="0"))
    float EffectiveRange = 6000.0f; // cm

	// Base yaw limit (0..360 sweep around initial yaw)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Limits",
	  meta=(ToolTip="Limit base yaw sweep around initial yaw (0=locked, 360=unlimited)"))
	bool bLimitBaseYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Limits",
	  meta=(ClampMin="0.0", ClampMax="360.0", ToolTip="Base yaw sweep range in degrees (0..360)"))
	float BaseYawLimitDeg;

	// Gun pitch limit (0..360 sweep around initial pitch)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Limits",
	  meta=(ToolTip="Limit gun pitch sweep around initial pitch (0=locked, 360=unlimited)"))
	bool bLimitGunPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Limits",
	  meta=(ClampMin="0.0", ClampMax="360.0", ToolTip="Gun pitch sweep range in degrees (0..360)"))
	float GunPitchLimitDeg;

	// Speed settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Speed",
	  meta=(ToolTip="If true, use deg/sec stepping instead of RInterpTo speeds"))
	bool bUseDegPerSecSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Speed",
	  meta=(ClampMin="0.0", ClampMax="720.0", ToolTip="Base yaw speed in deg/sec"))
	float YawDegPerSec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim|Speed",
	  meta=(ClampMin="0.0", ClampMax="720.0", ToolTip="Gun pitch speed in deg/sec"))
	float PitchDegPerSec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Speed", meta = (ClampMin = "0.0", ClampMax = "50.0", ToolTip = "Yaw interpolation speed"))
	float YawInterpSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Speed", meta = (ClampMin = "0.0", ClampMax = "50.0", ToolTip = "Pitch interpolation speed"))
	float PitchInterpSpeed;

	// Start point selection
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim", meta = (ToolTip = "Use muzzle location as start point for aim calculation"))
	bool bUseMuzzleAsStart;

	// Lead prediction settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim", meta=(ToolTip="Enable leading prediction for aiming"))
	bool bUseLeadPrediction = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aim", meta=(ClampMin="1.0", ToolTip="Bullet/projectile initial speed used for lead prediction (cm/s)"))
	float ProjectileInitialSpeed = 6000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shoot",
	  meta=(ToolTip="True when muzzle has clear line of sight to target on required channels"))
	bool ReadyToShoot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Shoot",
	  meta=(ToolTip="If false, ReadyToShoot always false"))
	bool bUseReadyToShootCheck = true;

	// Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Debug", meta = (ToolTip = "Enable debug visualization"))
	bool bDebugAim;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aim|Debug", meta = (ToolTip = "Duration for debug draw elements"))
	float DebugDrawDuration;

private:
	FTimerHandle AimTimerHandle;
	float DebugAccumTime;
	
	// Cached initial angles
	float InitialBaseYawWorld;
	float InitialGunPitchRel;

	bool HasLineOfSightToTarget() const;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called when the actor is being removed from the game
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	// Targeting functions
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void SetTargetActor(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Aim")
	void ClearTarget();

	// Manual aim control
	UFUNCTION(BlueprintCallable, Category = "Aim")
	void AimStepManual(float DeltaSeconds);

	// BlueprintPure getters
	UFUNCTION(BlueprintPure, Category = "Aim")
	FVector GetMuzzleWorldLocation() const;

	UFUNCTION(BlueprintPure, Category = "Aim")
	FVector GetMuzzleWorldForward() const;

	UFUNCTION(BlueprintPure, Category = "Aim")
	float GetCurrentYaw() const;

	UFUNCTION(BlueprintPure, Category = "Aim")
	float GetCurrentPitch() const;

	// Update functions
	void UpdateAim();
	void AimStep(float Dt);
};
