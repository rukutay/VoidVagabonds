#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "NavigationSubsystem.generated.h"

class ANavStaticBig;
class USphereComponent;

USTRUCT(BlueprintType)
struct FNavObstacleSphereProxy
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TWeakObjectPtr<AActor> Actor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	FVector Center = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	float BaseRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	float InflatedRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TWeakObjectPtr<USphereComponent> SignatureSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	bool bFromSignatureSphere = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation")
	TArray<FVector> Anchors;
};

UCLASS()
class VAGABONDSWORK_API UNavigationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	const TArray<FNavObstacleSphereProxy>& GetStaticNavObstacles() const;
	bool GetClosestObstacleToSegment(const FVector& A, const FVector& B, int32& OutIndex) const;
	bool IsSegmentClearOfStaticObstacles(const FVector& A, const FVector& B, int32* OutFirstHitIndex = nullptr, const AActor* IgnoredObstacleActor = nullptr) const;
	bool IsPointInsideNavStaticBigProxy(const FVector& Point, const AActor* IgnoredActor = nullptr, int32* OutObstacleIndex = nullptr) const;
	bool IsDirectPathBlockedByNavStaticBig(const FVector& A, const FVector& B, const AActor* IgnoredActor = nullptr, int32* OutObstacleIndex = nullptr) const;
	const FNavObstacleSphereProxy* FindObstacleByActor(const AActor* InActor) const;
	void SetRuntimeNavObstacleActors(const TArray<AActor*>& Actors);

	UFUNCTION(BlueprintCallable, Category = "Navigation")
	TArray<FVector> FindGlobalPathAnchors(const FVector& Start, const FVector& Goal, AActor* TargetActor = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "Navigation|Debug")
	bool DebugTestSegmentAgainstStaticObstacles(const FVector& A, const FVector& B);

	UFUNCTION(BlueprintCallable, Category = "Navigation|Debug")
	int32 GetStaticNavObstacleCount() const;

	UFUNCTION(BlueprintCallable, Category = "Navigation|Debug")
	void DebugDrawStaticNavObstacles(float Duration = 10.0f) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float NavSafetyMarginCm = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	int32 NavAnchorsPerObstacle = 24;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	float DefaultShipRadiusCm = 1500.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation")
	bool bNavDebugDrawStatic = false;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|Debug")
	bool bNavDebugDrawLineTrace = true;

	UPROPERTY(EditDefaultsOnly, Category = "Navigation|MovingStatic")
	float StaticObstacleRefreshInterval = 1.0f;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TArray<FNavObstacleSphereProxy> StaticNavObstacles;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TArray<FNavObstacleSphereProxy> RuntimeNavObstacles;

	UPROPERTY(VisibleAnywhere, Category = "Navigation")
	TArray<FNavObstacleSphereProxy> CombinedNavObstacles;

private:
	void HandlePostLoadMap(UWorld* LoadedWorld);
	void TryInitializeForWorld(UWorld* World);
	void EnsureNavObstaclesInitialized() const;

	void InitializeNavObstacles();
	void RefreshMovingStaticObstacles();
	void RefreshCombinedNavObstacles();
	void DrawStaticNavObstacles(float Duration) const;
	bool BuildNavStaticBigProxy(ANavStaticBig* Actor, FNavObstacleSphereProxy& OutProxy) const;
	bool BuildRuntimeObstacleProxy(AActor* Actor, FNavObstacleSphereProxy& OutProxy) const;
	void GenerateAnchorsForProxy(FNavObstacleSphereProxy& Proxy) const;
	bool ShouldSkipObstacleForSegment(const FNavObstacleSphereProxy& Proxy, const AActor* IgnoredActor) const;
	bool FindLineTraceNavStaticBigObstacle(const FVector& A, const FVector& B, const AActor* IgnoredActor, int32* OutObstacleIndex = nullptr) const;
	FVector ResolveEffectiveGoalForTargetObstacle(const FVector& Start, const FVector& RequestedGoal, const AActor* TargetActor) const;

	FTimerHandle MovingStaticRefreshTimer;
	FDelegateHandle PostLoadMapHandle;
	TWeakObjectPtr<UWorld> CachedNavWorld;
	bool bStaticNavObstaclesInitialized = false;
	bool bInitializingStaticNavObstacles = false;
};
