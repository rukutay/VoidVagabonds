// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"
#include "NavStaticBig.generated.h"

class UMarkerComponent;

UCLASS()
class VAGABONDSWORK_API ANavStaticBig : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANavStaticBig();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NavStaticBig|Custom")
	bool Custom = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField")
	bool AsteroidField = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Planet")
	UStaticMeshComponent* BodyMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Planet")
	UMarkerComponent* MarkerComponent = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField")
	USphereComponent* SignatureSphere = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AsteroidField")
	USplineComponent* FieldSpline = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="AsteroidField")
	UHierarchicalInstancedStaticMeshComponent* AsteroidHISM = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Config")
	float FieldWidth = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Config")
	float FieldHeight = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Config")
	float DensityPer1000uu = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	bool bEnableStreaming = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingRadius = 20000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float NearSpawnProbability = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StepJitterMin = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StepJitterMax = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingStepJitter = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingDropoutMin = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingDropoutMax = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingFrameRollStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingDistanceWarp = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingNoiseFrequency = 0.0005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingRadialNoiseAmplitude = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingRadialNoiseFrequency = 0.0007f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingClusterChance = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	int32 StreamingClusterMaxExtra = 2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingClusterRadius = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingRebuildThreshold = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingChunkLength = 5000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingBandHysteresis = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	float StreamingUpdateInterval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	int32 MaxAsteroidInstances = 20000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	int32 MaxNearInstances = 12000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	int32 MaxInstancesPerUpdate = 5000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Streaming")
	int32 PreviewMaxInstancesEditor = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Collision")
	bool bEnableNearCollision = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Collision")
	bool bSpawnDynamicOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|NearSwap")
	bool bEnableNearActorSwap = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|NearSwap")
	float NearActorSwapEnterRadius = 15000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|NearSwap")
	float NearActorSwapExitRadius = 18000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|NearSwap")
	float NearActorSwapInterval = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|NearSwap")
	bool bDebugNearActorSwap = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Collision")
	TSubclassOf<AActor> DynamicAsteroidClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Collision")
	int32 MaxDynamicAsteroids = 40;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Collision")
	float HitImpulseStrength = 50000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Config")
	float MinAsteroidScale = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Config")
	float MaxAsteroidScale = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Config")
	int32 Seed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Meshes")
	TArray<UStaticMesh*> AsteroidMeshes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AsteroidField|Spline")
	float SplineRadiusOverride = 0.0f;

	UFUNCTION(BlueprintCallable, Category="AsteroidField")
	void GenerateAsteroidField();

	UFUNCTION(BlueprintCallable, CallInEditor, Category="AsteroidField|Spline")
	void BuildCircularSpline(float Radius = 0.0f, int32 NumPoints = 16);

	UFUNCTION(BlueprintCallable, Category="AsteroidField|Collision")
	bool ReplaceHISMInstanceWithActor(UHierarchicalInstancedStaticMeshComponent* SourceHISM, int32 InstanceIndex);

private:
	struct FNearSwapEntry
	{
		TWeakObjectPtr<UHierarchicalInstancedStaticMeshComponent> SourceHISM;
		FTransform InstanceTransform;
		bool bBecameDynamic = false;
	};

	struct FStreamingChunk
	{
		UHierarchicalInstancedStaticMeshComponent* Near = nullptr;
		int32 LastBand = INDEX_NONE;
	};

	void UpdateAsteroidStreaming();
	void UpdateNearAsteroidActorSwap();
	void CleanupNearSwapForComponent(UHierarchicalInstancedStaticMeshComponent* Component);
	FVector GetStreamingViewLocation() const;
	UStaticMesh* GetRandomAsteroidMesh(FRandomStream& RandomStream) const;
	bool SpawnDynamicAsteroidFromInstance(UHierarchicalInstancedStaticMeshComponent* SourceHISM, int32 InstanceIndex,
		const FVector& NormalImpulse, bool bApplyImpulse);
	bool SpawnNearSwapAsteroidFromInstance(UHierarchicalInstancedStaticMeshComponent* SourceHISM, int32 InstanceIndex);
	bool RestoreNearSwapActor(AActor* SwappedActor, bool bDestroyActor);

	UFUNCTION()
	void OnAsteroidHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	void OnDynamicAsteroidDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnNearSwapAsteroidDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void OnNearSwapAsteroidWake(UPrimitiveComponent* WokeComponent, FName BoneName);

	FTimerHandle StreamingTimerHandle;
	FTimerHandle NearSwapTimerHandle;
	int32 ActiveDynamicAsteroids = 0;
	float LastStreamingCenterDistance = -1.0f;
	uint32 LastStreamingConfigHash = 0;
	TMap<int32, FStreamingChunk> ActiveStreamingChunks;
	TSet<UHierarchicalInstancedStaticMeshComponent*> StreamingChunkComponents;
	TSet<UHierarchicalInstancedStaticMeshComponent*> NearSwapHISMComponents;
	TMap<TWeakObjectPtr<AActor>, FNearSwapEntry> ActiveNearSwapActors;
	bool bRestoringNearSwapActor = false;
};
