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
};
