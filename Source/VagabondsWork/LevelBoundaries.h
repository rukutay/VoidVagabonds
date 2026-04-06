// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "LevelBoundaries.generated.h"

class USphereComponent;
class ANavStaticBig;

USTRUCT(BlueprintType)
struct FAtmosphereSpawnConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere")
	TSubclassOf<AActor> AtmosphereClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere", meta=(ClampMin="100000.0"))
	float ExpectedSphereRadius = 110000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere", meta=(ClampMin="0.0"))
	float PredictionTimeSec = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere", meta=(ClampMin="0.0"))
	float SpawnJitterRadius = 25000.0f;
};

USTRUCT()
struct FAtmosphereRuntimeEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<AActor> AtmosphereClass;

	UPROPERTY()
	TWeakObjectPtr<AActor> SpawnedActor;

	UPROPERTY()
	float EffectiveRadius = 110000.0f;
};

UCLASS()
class VAGABONDSWORK_API ALevelBoundaries : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ALevelBoundaries();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Custom")
	bool Custom = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boundaries")
	USphereComponent* Boudaries = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere")
	TArray<FAtmosphereSpawnConfig> AtmosphereActors;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere")
	bool bEnableAtmosphereSpawning = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere", meta=(ClampMin="1", ClampMax="200"))
	int32 MaxAtmosphereInstances = 200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere", meta=(ClampMin="0.1"))
	float EnvironmentControlIntervalSec = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere", meta=(ClampMin="1.0"))
	float SpawnMinIntervalSec = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere", meta=(ClampMin="1.0"))
	float SpawnMaxIntervalSec = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boundaries|Atmosphere", meta=(ClampMin="0.0"))
	float DespawnDistanceMultiplier = 2.0f;

	UPROPERTY(Transient)
	TArray<FAtmosphereRuntimeEntry> ActiveAtmosphereEntries;

	UFUNCTION(BlueprintCallable, Category="Boundaries")
	void NavStaticBig(TSubclassOf<ANavStaticBig> NavStaticBigClass, int32 MinNum, int32 MaxNum, float MinDistance);

	UFUNCTION(BlueprintCallable, Category="Boundaries")
	float GetBoundaryRadius() const;

private:
	void StartAtmosphereControl();
	void HandleAtmosphereControlTick();
	AActor* GetPrimaryPlayerActor() const;
	bool SpawnInitialAtmosphereAtPlayer();
	void CleanupDestroyedAtmosphereEntries();
	void DespawnIrrelevantAtmosphere(const FVector& PlayerLocation);
	bool IsPlayerInsideAnyAtmosphere(const FVector& PlayerLocation) const;
	bool TrySpawnAtmosphereForTravel(const FVector& PlayerLocation, const FVector& PlayerVelocity);
	FVector ComputePredictedSpawnLocation(const FAtmosphereSpawnConfig& Config, const FVector& PlayerLocation, const FVector& PlayerVelocity) const;
	bool CanSpawnWithoutSameClassOverlap(TSubclassOf<AActor> Class, const FVector& CandidateLocation, float CandidateRadius) const;
	float ResolveActorSphereRadiusOrDefault(AActor* Actor, float FallbackRadius) const;
	bool SpawnAtmosphereAtLocation(const FAtmosphereSpawnConfig& Config, const FVector& Location);

	FTimerHandle AtmosphereControlTimerHandle;
	float NextAtmosphereSpawnTime = 0.0f;

};
