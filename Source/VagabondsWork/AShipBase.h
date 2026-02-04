// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "AShipBase.generated.h"

UCLASS()
class VAGABONDSWORK_API AAShipBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAShipBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

    /** Visual base of the ship (ROOT) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    UStaticMeshComponent* ShipBase;

};
