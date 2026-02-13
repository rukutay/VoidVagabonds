// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerMainController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UMapWidget;

/**
 * 
 */
UCLASS()
class VAGABONDSWORK_API APlayerMainController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintCallable, Category = "Input")
	void SwitchToPawn(APawn* PawnToControl);

	UFUNCTION(BlueprintImplementableEvent, Category = "Input")
	void OnSwitchControlRequested();

protected:
	void HandleThrottleInput(const FInputActionValue& Value);
	void HandlePitchInput(const FInputActionValue& Value);
	void HandleYawInput(const FInputActionValue& Value);
	void HandleRollInput(const FInputActionValue& Value);
	void HandleSpectatorMoveInput(const FInputActionValue& Value);
	void HandleSpectatorLookInput(const FInputActionValue& Value);
	void HandleSwitchControlInput(const FInputActionValue& Value);
	void UpdateShipRotationInput();
	void UpdateShipMappingContext(APawn* InPawn);

private:
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* ShipMappingContext = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputMappingContext* SpectatorMappingContext = nullptr;

	UPROPERTY(EditAnywhere, Category = "UI")
	TSubclassOf<UMapWidget> MapWidgetClass;

	UPROPERTY(Transient)
	UMapWidget* MapWidgetInstance = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ThrottleAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* PitchAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* YawAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* RollAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SpectatorMoveAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SpectatorLookAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SwitchControlAction = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<class APlayerSpectator> CachedSpectatorPawn;

	float CachedPitchInput = 0.f;
	float CachedYawInput = 0.f;
	float CachedRollInput = 0.f;
};
