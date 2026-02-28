// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MarkerComponent.h"
#include "VagabondsGameInstance.generated.h"

UENUM(BlueprintType)
enum class EActorFilterKind : uint8
{
	Faction,
	MarkerType
};

UCLASS()
class VAGABONDSWORK_API UVagabondsGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void OnStart() override;

	UFUNCTION(BlueprintCallable, Category = "World|Actors")
	TArray<AActor*> GetActorsByFilter(const TArray<AActor*>& Actors, EActorFilterKind FilterKind, uint8 EnumValue) const;
};
