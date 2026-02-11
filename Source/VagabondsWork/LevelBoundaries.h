// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelBoundaries.generated.h"

class USphereComponent;
class ANavStaticBig;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Boundaries")
	USphereComponent* Boudaries = nullptr;

	UFUNCTION(BlueprintCallable, Category="Boundaries")
	void NavStaticBig(TSubclassOf<ANavStaticBig> NavStaticBigClass, int32 MinNum, int32 MaxNum, float MinDistance);

};
