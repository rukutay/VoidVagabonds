#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FactionsSubsystem.h"
#include "LevelActorsSubsystem.generated.h"

class ANavStaticBig;
class AShip;

UCLASS()
class VAGABONDSWORK_API ULevelActorsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<AActor*> GetStationsAll() const;

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<ANavStaticBig*> GetPlanetsAll() const;

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<AShip*> GetShipsAll() const;

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<AActor*> GetStationsOfFaction(EFaction Faction) const;

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<ANavStaticBig*> GetPlanetsOfFaction(EFaction Faction) const;

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<AShip*> GetShipsOfFaction(EFaction Faction) const;

private:
	void RefreshTrackedActors();

	UPROPERTY(EditDefaultsOnly, Category = "World|Actors")
	float ActorListsRefreshInterval = 0.05f;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> Stations;

	UPROPERTY()
	TArray<TObjectPtr<ANavStaticBig>> Planets;

	UPROPERTY()
	TArray<TObjectPtr<AShip>> Ships;

	FTimerHandle TrackedActorsRefreshTimer;
};