// Fill out your copyright notice in the Description page of Project Settings.


#include "NavStaticBig.h"

// Sets default values
ANavStaticBig::ANavStaticBig()
{
	PrimaryActorTick.bCanEverTick = false;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	SetRootComponent(BodyMesh);

	BodyMesh->LightingChannels.bChannel0 = true;
	BodyMesh->LightingChannels.bChannel1 = false;
	BodyMesh->LightingChannels.bChannel2 = false;
}

// Called when the game starts or when spawned
void ANavStaticBig::BeginPlay()
{
	Super::BeginPlay();
}

