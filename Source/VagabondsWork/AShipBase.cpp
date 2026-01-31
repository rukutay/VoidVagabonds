// Fill out your copyright notice in the Description page of Project Settings.


#include "AShipBase.h"

// Sets default values
AAShipBase::AAShipBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    /** Ship base mesh — ROOT */
    ShipBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShipBase"));
    SetRootComponent(ShipBase);

    ShipBase->SetSimulatePhysics(false);
    ShipBase->SetCollisionProfileName(TEXT("Pawn"));

    /** Collision sphere */
    ShipRadius = CreateDefaultSubobject<USphereComponent>(TEXT("ShipRadius"));
    ShipRadius->SetupAttachment(ShipBase);

    ShipRadius->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    ShipRadius->SetCollisionProfileName(TEXT("Pawn"));
    ShipRadius->SetSphereRadius(300.f);

}

// Called when the game starts or when spawned
void AAShipBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAShipBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

