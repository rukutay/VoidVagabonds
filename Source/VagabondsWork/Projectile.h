// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShipVitalityComponent.h" // for FShipDamageSpec / EDamageType
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Projectile.generated.h"

UCLASS()
class VAGABONDSWORK_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Component references
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	UStaticMeshComponent* ProjectileMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	USphereComponent* Collision = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile")
	UProjectileMovementComponent* Movement = nullptr;

	// Damage properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|Damage")
	float DamageAmount = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|Damage")
	EDamageType DamageType = EDamageType::DT_Kinetic;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|Damage")
	bool bBypassShield = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile|Damage", meta=(ClampMin="0.0", ClampMax="1.0"))
	float ArmorPenetration = 0.0f;

	// Behavior properties
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile")
	bool bDestroyOnOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Projectile", meta=(ClampMin="0.0"))
	float LifeSeconds = 10.f;

	// Overlap handler
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
					   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
					   bool bFromSweep, const FHitResult& SweepResult);

	// Damage spec creation
	UFUNCTION(BlueprintCallable, Category="Projectile|Damage")
	FShipDamageSpec MakeDamageSpec() const;
};
