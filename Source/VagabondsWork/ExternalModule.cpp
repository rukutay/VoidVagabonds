// Fill out your copyright notice in the Description page of Project Settings.

#include "ExternalModule.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

// Sets default values
AExternalModule::AExternalModule()
{
	// Set this actor to not call Tick() every frame to improve performance
	PrimaryActorTick.bCanEverTick = false;

	// Create core component hierarchy
	ModuleRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ModuleRoot"));
	SetRootComponent(ModuleRoot);

	BaseYawPivot = CreateDefaultSubobject<USceneComponent>(TEXT("BaseYawPivot"));
	BaseYawPivot->SetupAttachment(ModuleRoot);

	BaseMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseMesh"));
	BaseMesh->SetupAttachment(BaseYawPivot);

	GunPitchPivot = CreateDefaultSubobject<USceneComponent>(TEXT("GunPitchPivot"));
	GunPitchPivot->SetupAttachment(BaseYawPivot);

	GunMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GunMesh"));
	GunMesh->SetupAttachment(GunPitchPivot);

	Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
	Muzzle->SetupAttachment(GunPitchPivot);

}

// Called when the game starts or when spawned
void AExternalModule::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called when the actor is being removed from the game
void AExternalModule::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}