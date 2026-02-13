// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerMainController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Ship.h"
#include "Engine/LocalPlayer.h"
#include "MapWidget.h"
#include "PlayerSpectator.h"

void APlayerMainController::BeginPlay()
{
    Super::BeginPlay();

    UpdateShipMappingContext(GetPawn());

    if (MapWidgetClass && !MapWidgetInstance)
    {
        MapWidgetInstance = CreateWidget<UMapWidget>(this, MapWidgetClass);
        if (MapWidgetInstance)
        {
            MapWidgetInstance->AddToViewport();
        }
    }
}

void APlayerMainController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if (APlayerSpectator* SpectatorPawn = Cast<APlayerSpectator>(InPawn))
    {
        CachedSpectatorPawn = SpectatorPawn;
    }
    UpdateShipMappingContext(InPawn);
}

void APlayerMainController::OnUnPossess()
{
    Super::OnUnPossess();

    UpdateShipMappingContext(nullptr);
}

void APlayerMainController::UpdateShipMappingContext(APawn* InPawn)
{
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (ShipMappingContext)
            {
                Subsystem->RemoveMappingContext(ShipMappingContext);
            }
            if (SpectatorMappingContext)
            {
                Subsystem->RemoveMappingContext(SpectatorMappingContext);
            }
            if (InPawn && InPawn->IsA<AShip>())
            {
                if (ShipMappingContext)
                {
                    Subsystem->AddMappingContext(ShipMappingContext, 1);
                }
            }
            else if (InPawn && InPawn->IsA<APlayerSpectator>())
            {
                if (SpectatorMappingContext)
                {
                    Subsystem->AddMappingContext(SpectatorMappingContext, 0);
                }
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

    if (ThrottleAction)
    {
        EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &APlayerMainController::HandleThrottleInput);
        EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &APlayerMainController::HandleThrottleInput);
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
    if (SpectatorMoveAction)
    {
        EnhancedInputComponent->BindAction(SpectatorMoveAction, ETriggerEvent::Triggered, this, &APlayerMainController::HandleSpectatorMoveInput);
    }
    if (SpectatorLookAction)
    {
        EnhancedInputComponent->BindAction(SpectatorLookAction, ETriggerEvent::Triggered, this, &APlayerMainController::HandleSpectatorLookInput);
    }
    if (SwitchControlAction)
    {
        EnhancedInputComponent->BindAction(SwitchControlAction, ETriggerEvent::Triggered, this, &APlayerMainController::HandleSwitchControlInput);
    }
}

void APlayerMainController::HandleThrottleInput(const FInputActionValue& Value)
{
    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->SetManualThrottleInput(Value.Get<float>());
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

void APlayerMainController::HandleSpectatorMoveInput(const FInputActionValue& Value)
{
    if (APlayerSpectator* Spectator = Cast<APlayerSpectator>(GetPawn()))
    {
        Spectator->ApplyMoveInput(Value);
    }
}

void APlayerMainController::HandleSpectatorLookInput(const FInputActionValue& Value)
{
    if (APlayerSpectator* Spectator = Cast<APlayerSpectator>(GetPawn()))
    {
        Spectator->ApplyLookInput(Value);
    }
}

void APlayerMainController::HandleSwitchControlInput(const FInputActionValue& Value)
{
    if (Value.Get<bool>())
    {
        OnSwitchControlRequested();
    }
}

void APlayerMainController::UpdateShipRotationInput()
{
    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        Ship->SetManualRotationInput(CachedPitchInput, CachedYawInput, CachedRollInput);
    }
}

void APlayerMainController::SwitchToPawn(APawn* PawnToControl)
{
    if (!PawnToControl)
    {
        return;
    }

    if (PawnToControl == GetPawn())
    {
        if (CachedSpectatorPawn.IsValid())
        {
            if (AShip* Ship = Cast<AShip>(GetPawn()))
            {
                CachedSpectatorPawn->SyncToTransform(Ship->GetShipCameraTransform());
                CachedSpectatorPawn->SetCameraBoomLength(0.0f);
            }
            CachedSpectatorPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            Possess(CachedSpectatorPawn.Get());
        }
        return;
    }

    if (APlayerSpectator* SpectatorPawn = Cast<APlayerSpectator>(GetPawn()))
    {
        CachedSpectatorPawn = SpectatorPawn;
        SpectatorPawn->SyncToTransform(PawnToControl->GetActorTransform());
        SpectatorPawn->AttachToActor(PawnToControl, FAttachmentTransformRules::KeepWorldTransform);
    }

    Possess(PawnToControl);
}

