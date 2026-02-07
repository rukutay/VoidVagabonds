// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerMainController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Ship.h"
#include "Engine/LocalPlayer.h"

void APlayerMainController::BeginPlay()
{
    Super::BeginPlay();

    if (ShipMappingContext)
    {
        if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Subsystem->AddMappingContext(ShipMappingContext, 1);
            }
        }
    }
}

void APlayerMainController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
    if (!EnhancedInputComponent)
    {
        return;
    }

    if (ThrottleUpAction)
    {
        EnhancedInputComponent->BindAction(ThrottleUpAction, ETriggerEvent::Started, this, &APlayerMainController::HandleThrottleUp);
    }
    if (ThrottleDownAction)
    {
        EnhancedInputComponent->BindAction(ThrottleDownAction, ETriggerEvent::Started, this, &APlayerMainController::HandleThrottleDown);
    }
    if (PitchAction)
    {
        EnhancedInputComponent->BindAction(PitchAction, ETriggerEvent::Triggered, this, &APlayerMainController::HandlePitchInput);
        EnhancedInputComponent->BindAction(PitchAction, ETriggerEvent::Completed, this, &APlayerMainController::HandlePitchInput);
    }
    if (YawAction)
    {
        EnhancedInputComponent->BindAction(YawAction, ETriggerEvent::Triggered, this, &APlayerMainController::HandleYawInput);
        EnhancedInputComponent->BindAction(YawAction, ETriggerEvent::Completed, this, &APlayerMainController::HandleYawInput);
    }
    if (RollAction)
    {
        EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Triggered, this, &APlayerMainController::HandleRollInput);
        EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Completed, this, &APlayerMainController::HandleRollInput);
    }
}

void APlayerMainController::HandleThrottleUp(const FInputActionValue& Value)
{
    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->StepThrottleUp();
    }
}

void APlayerMainController::HandleThrottleDown(const FInputActionValue& Value)
{
    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->StepThrottleDown();
    }
}

void APlayerMainController::HandlePitchInput(const FInputActionValue& Value)
{
    CachedPitchInput = Value.Get<float>();
    UpdateShipRotationInput();
}

void APlayerMainController::HandleYawInput(const FInputActionValue& Value)
{
    CachedYawInput = Value.Get<float>();
    UpdateShipRotationInput();
}

void APlayerMainController::HandleRollInput(const FInputActionValue& Value)
{
    CachedRollInput = Value.Get<float>();
    UpdateShipRotationInput();
}

void APlayerMainController::UpdateShipRotationInput()
{
    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->SetManualRotationInput(CachedPitchInput, CachedYawInput, CachedRollInput);
    }
}

