#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Ship.generated.h"

class AAIShipController;
class UShipNavComponent;

UCLASS()
class VAGABONDSWORK_API AShip : public APawn
{
    GENERATED_BODY()

public:
    AShip();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    // NAVIGATION_TODO_REMOVE
    UFUNCTION(BlueprintCallable, Category="Ship|AI")
    FVector GetGoalLocation() const;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="AI|Navigation")
    AActor* TargetActor = nullptr;

    UFUNCTION(BlueprintCallable, Category="AI|Navigation")
    AActor* GetTargetActor() const { return TargetActor; }

    // ---------------- Controller ----------------
    // NAVIGATION_TODO_REMOVE
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI", meta=(AllowPrivateAccess="true"))
    AAIShipController* ShipController = nullptr;

    // ---------------- Components ----------------
    UFUNCTION(BlueprintCallable)
    UStaticMeshComponent* GetShipBase() const;

    // ---------------- AI Controls ----------------
    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxPitchSpeed = 6.f;   // deg/sec

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxYawSpeed = 18.f;    // deg/sec

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float PitchAccelSpeed = 6.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float YawAccelSpeed = 7.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxRollSpeed = 20.f;   // deg/sec

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float RollAccelSpeed = 10.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Rotation")
    float MaxRollAngle = 22.5f; // degrees

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    UStaticMeshComponent* ShipBase = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|Components")
    USphereComponent* ShipRadius = nullptr;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI")
    UShipNavComponent* ShipNav = nullptr;

    // ---------------- Movement ----------------
    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float MaxForwardForce = 500.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float MinThrottle = 0.15f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ship|AI|Movement")
    float ThrottleInterpSpeed = 2.5f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Movement")
    float CurrentThrottle = 0.f;

    // NAVIGATION_TODO_REMOVE
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Ship|AI|Movement")
    float LateralDamping = 2.5f;

    // NAVIGATION_TODO_REMOVE
    UFUNCTION(BlueprintCallable)
    void ApplySteeringForce(FVector TargetLocation, float DeltaTime);

private:
    bool EnsureShipController();

    bool bLoggedMissingController = false;

    float DebugMessageAccumulator = 0.f;
};
