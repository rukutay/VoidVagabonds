// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerMainController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

/**
 * 
 */
UCLASS()
class VAGABONDSWORK_API APlayerMainController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

protected:
	void HandleThrottleUp(const FInputActionValue& Value);
	void HandleThrottleDown(const FInputActionValue& Value);
	void HandlePitchInput(const FInputActionValue& Value);
	void HandleYawInput(const FInputActionValue& Value);
	void HandleRollInput(const FInputActionValue& Value);
	void UpdateShipRotationInput();

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* ShipMappingContext = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ThrottleUpAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ThrottleDownAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* PitchAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* YawAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* RollAction = nullptr;

	float CachedPitchInput = 0.f;
	float CachedYawInput = 0.f;
	float CachedRollInput = 0.f;
};
