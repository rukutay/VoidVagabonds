// Fill out your copyright notice in the Description page of Project Settings.

#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(Collision);
	ProjectileMesh->SetSimulatePhysics(false);
	ProjectileMesh->SetEnableGravity(false);

	Collision->InitSphereRadius(5.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_GameTraceChannel1);
	Collision->SetCollisionResponseToAllChannels(ECR_Overlap);
	Collision->SetGenerateOverlapEvents(true);

	Collision->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnBeginOverlap);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->InitialSpeed = 6000.f;
	Movement->MaxSpeed = 6000.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bShouldBounce = false;
	Movement->ProjectileGravityScale = 0.f; // space
}

// Called when the game starts or when spawned
void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Movement)
	{
		Movement->Velocity = GetActorForwardVector() * Movement->InitialSpeed;
		Movement->Activate(true);
	}

	if (LifeSeconds > 0.f)
	{
		SetLifeSpan(LifeSeconds);
	}

	// Ignore owner collision
	if (AActor* Owner = GetOwner())
	{
		Collision->IgnoreActorWhenMoving(Owner, true);
		Collision->MoveIgnoreActors.AddUnique(Owner);
	}
}

FShipDamageSpec AProjectile::MakeDamageSpec() const
{
	FShipDamageSpec Spec;
	Spec.Amount = DamageAmount;
	Spec.Type = DamageType;
	Spec.bBypassShield = bBypassShield;
	Spec.ArmorPenetration = ArmorPenetration;
	Spec.Instigator = GetOwner();              // "who fired"
	Spec.HitLocation = GetActorLocation();     // best available here
	return Spec;
}

void AProjectile::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
								UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
								bool bFromSweep, const FHitResult& SweepResult)
{
	// Validate OtherActor
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	} 

	// Find vitality component
	UShipVitalityComponent* Vitality = OtherActor->FindComponentByClass<UShipVitalityComponent>();
	if (!Vitality)
	{
		return;
	}

	// Apply damage
	FShipDamageSpec Spec = MakeDamageSpec();
	Spec.HitLocation = SweepResult.ImpactPoint.IsNearlyZero() ? GetActorLocation() : FVector(SweepResult.ImpactPoint);
	Vitality->ApplyDamage(Spec);

	// Destroy if requested
	if (bDestroyOnOverlap)
	{
		Destroy();
	}
}

