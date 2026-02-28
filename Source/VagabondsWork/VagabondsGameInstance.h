// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"
#include "MarkerComponent.h"
#include "VagabondsGameInstance.generated.h"

UENUM(BlueprintType)
enum class EActorFilterKind : uint8
{
	Faction,
	MarkerType
};

class ANavStaticBig;
class AShip;

UCLASS()
class VAGABONDSWORK_API UVagabondsGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void OnStart() override;

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<AActor*> GetActorsByFilter(const TArray<AActor*>& Actors, EActorFilterKind FilterKind, uint8 EnumValue) const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Actors")
	TArray<TObjectPtr<ANavStaticBig>> AllPlanets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World|Actors")
	TArray<TObjectPtr<AShip>> AllShips;

	UPROPERTY(EditDefaultsOnly, Category = "World|Actors")
	float ActorListsRefreshInterval = 0.05f;

private:
	void RefreshTrackedActors();

	FTimerHandle TrackedActorsRefreshTimer;
};
