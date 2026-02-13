// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "PlayerSpectator.generated.h"

class USphereComponent;
class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;
struct FInputActionValue;

UCLASS()
class VAGABONDSWORK_API APlayerSpectator : public APawn
{
	GENERATED_BODY()

public:
	APlayerSpectator();
	void ApplyMoveInput(const FInputActionValue& Value);
	void ApplyLookInput(const FInputActionValue& Value);
	void SetCameraBoomLength(float NewLength);
	float GetCameraBoomLength() const;
	void SyncToTransform(const FTransform& NewTransform);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Spectator|Components")
	USphereComponent* RootSphere = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Spectator|Components")
	USpringArmComponent* CameraBoom = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Spectator|Components")
	UCameraComponent* SpectatorCamera = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Spectator|Components")
	UFloatingPawnMovement* Movement = nullptr;

	UPROPERTY(EditAnywhere, Category = "Spectator|Movement")
	float MaxSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Spectator|Movement")
	float Acceleration = 8000.0f;

	UPROPERTY(EditAnywhere, Category = "Spectator|Movement")
	float Deceleration = 8000.0f;

	UPROPERTY(EditAnywhere, Category = "Spectator|Look")
	float LookSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Spectator|Look")
	float RotationInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, Category = "Spectator|Look")
	float PitchClampMin = -80.0f;

	UPROPERTY(EditAnywhere, Category = "Spectator|Look")
	float PitchClampMax = 80.0f;

	FRotator TargetRotation = FRotator::ZeroRotator;
};