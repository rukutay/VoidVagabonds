// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerMainController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Ship.h"
#include "Camera/CameraComponent.h"
#include "Engine/LocalPlayer.h"
#include "MapWidget.h"
#include "PlayerSpectator.h"
#include "GameFramework/SpringArmComponent.h"
#include "TimerManager.h"

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

    ClearCameraReset();
    if (APlayerSpectator* SpectatorPawn = Cast<APlayerSpectator>(InPawn))
    {
        CachedSpectatorPawn = SpectatorPawn;
    }
    UpdateShipMappingContext(InPawn);
}

void APlayerMainController::OnUnPossess()
{
    Super::OnUnPossess();

    ClearCameraReset();
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
    if (ToggleCursorAction)
    {
        EnhancedInputComponent->BindAction(ToggleCursorAction, ETriggerEvent::Triggered, this, &APlayerMainController::HandleToggleCursorInput);
    }
    if (LookOverrideAction)
    {
        EnhancedInputComponent->BindAction(LookOverrideAction, ETriggerEvent::Started, this, &APlayerMainController::HandleLookOverrideInput);
        EnhancedInputComponent->BindAction(LookOverrideAction, ETriggerEvent::Completed, this, &APlayerMainController::HandleLookOverrideInput);
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
    if (!IsLookInputAllowed())
    {
        return;
    }

    if (APlayerSpectator* Spectator = Cast<APlayerSpectator>(GetPawn()))
    {
        Spectator->ApplyLookInput(Value);
		return;
    }

	if (Cast<AShip>(GetPawn()))
	{
		const FVector2D LookAxis = Value.Get<FVector2D>();
		if (!LookAxis.IsNearlyZero())
		{
			ClearCameraReset();
			ScheduleCameraReset();
		}
		AddYawInput(LookAxis.X);
		AddPitchInput(LookAxis.Y);
	}
}

void APlayerMainController::HandleSwitchControlInput(const FInputActionValue& Value)
{
    if (Value.Get<bool>())
    {
        OnSwitchControlRequested();
    }
}

void APlayerMainController::HandleToggleCursorInput(const FInputActionValue& Value)
{
    if (!Value.Get<bool>())
    {
        return;
    }

    bCursorVisibleMode = !bCursorVisibleMode;
    bShowMouseCursor = bCursorVisibleMode;

    if (bCursorVisibleMode)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
    }
    else
    {
        SetInputMode(FInputModeGameOnly());
    }
}

void APlayerMainController::HandleLookOverrideInput(const FInputActionValue& Value)
{
    bLookOverrideHeld = Value.Get<bool>();

    if (bLookOverrideHeld)
    {
        SetInputMode(FInputModeGameOnly());
    }
    else if (bCursorVisibleMode)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);
    }
    else
    {
        SetInputMode(FInputModeGameOnly());
    }
}

void APlayerMainController::UpdateShipRotationInput()
{
    if (AShip* Ship = Cast<AShip>(GetPawn()))
    {
        if (IsLookInputAllowed())
        {
            Ship->SetManualRotationInput(CachedPitchInput, CachedYawInput, CachedRollInput);
        }
        else
        {
            Ship->SetManualRotationInput(0.f, 0.f, 0.f);
        }
    }
}

bool APlayerMainController::IsLookInputAllowed() const
{
    return !bCursorVisibleMode || bLookOverrideHeld;
}

void APlayerMainController::ScheduleCameraReset()
{
	if (CameraResetDelay <= 0.0f)
	{
		BeginCameraReset();
		return;
	}

	GetWorldTimerManager().SetTimer(CameraResetDelayHandle, this, &APlayerMainController::BeginCameraReset, CameraResetDelay, false);
}

void APlayerMainController::BeginCameraReset()
{
	if (bCameraResetActive || !Cast<AShip>(GetPawn()))
	{
		return;
	}

	bCameraResetActive = true;
	GetWorldTimerManager().SetTimer(CameraResetInterpHandle, this, &APlayerMainController::UpdateCameraReset, 0.02f, true);
}

void APlayerMainController::UpdateCameraReset()
{
	AShip* Ship = Cast<AShip>(GetPawn());
	if (!Ship)
	{
		ClearCameraReset();
		return;
	}

	const FRotator TargetRotation = Ship->GetActorRotation();
	const FRotator CurrentRotation = GetControlRotation();
	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 5.0f);
	SetControlRotation(NewRotation);

	if (NewRotation.Equals(TargetRotation, 0.1f))
	{
		SetControlRotation(TargetRotation);
		ClearCameraReset();
	}
}

void APlayerMainController::ClearCameraReset()
{
	GetWorldTimerManager().ClearTimer(CameraResetDelayHandle);
	GetWorldTimerManager().ClearTimer(CameraResetInterpHandle);
	bCameraResetActive = false;
}

void APlayerMainController::BeginLookAtAttachBlend(AActor* LookAt)
{
	APlayerSpectator* SpectatorPawn = CachedSpectatorPawn.Get();
	if (!SpectatorPawn || !LookAt)
	{
		return;
	}

	ClearLookAtAttachBlend();

	LookAtAttachTargetActor = LookAt;
	LookAtAttachBlendElapsed = 0.0f;
	bLookAtAttachBlendActive = true;

	if (SpectatorPawn->GetAttachParentActor())
	{
		SpectatorPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}

	if (USceneComponent* RootComponent = SpectatorPawn->GetRootComponent())
	{
		RootComponent->SetUsingAbsoluteRotation(false);
	}

	const USpringArmComponent* SpectatorArm = SpectatorPawn->FindComponentByClass<USpringArmComponent>();
	const FTransform StartCameraTransform = SpectatorArm
		? SpectatorArm->GetSocketTransform(USpringArmComponent::SocketName)
		: SpectatorPawn->GetActorTransform();
	LookAtAttachStartTransform = FTransform(StartCameraTransform.GetRotation(), StartCameraTransform.GetLocation(), FVector::OneVector);
	LookAtAttachStartCameraLocation = SpectatorPawn->GetActorLocation();
	LookAtAttachTargetCameraLocation = LookAt->GetActorLocation();
	LookAtAttachStartArmLength = SpectatorPawn->GetCameraBoomLength();
	if (const USpringArmComponent* TargetArm = LookAt->FindComponentByClass<USpringArmComponent>())
	{
		LookAtAttachTargetArmLength = TargetArm->TargetArmLength;
	}
	else
	{
		LookAtAttachTargetArmLength = LookAtAttachStartArmLength;
	}
	bLookAtAttachBlendIsAttach = true;
	SpectatorPawn->SyncToTransform(LookAtAttachStartTransform);

	if (LookAtAttachBlendDuration <= 0.0f)
	{
		LookAtAttachBlendElapsed = LookAtAttachBlendDuration;
		UpdateLookAtAttachBlend();
		return;
	}

	GetWorldTimerManager().SetTimer(LookAtAttachBlendHandle, this, &APlayerMainController::UpdateLookAtAttachBlend, 0.005f, true);
}


void APlayerMainController::UpdateLookAtAttachBlend()
{
	if (!bLookAtAttachBlendActive)
	{
		return;
	}

	APlayerSpectator* SpectatorPawn = CachedSpectatorPawn.Get();
	AActor* TargetActor = LookAtAttachTargetActor.Get();
	if (!SpectatorPawn || !TargetActor)
	{
		ClearLookAtAttachBlend();
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	LookAtAttachBlendElapsed += DeltaSeconds;
	const float Duration = FMath::Max(LookAtAttachBlendDuration, 0.0f);
	const float Alpha = Duration > 0.0f
		? FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(LookAtAttachBlendElapsed / Duration, 0.0f, 1.0f))
		: 1.0f;
	const float NewArmLength = FMath::Lerp(LookAtAttachStartArmLength, LookAtAttachTargetArmLength, Alpha);
	SpectatorPawn->SetCameraBoomLength(NewArmLength);
	const FVector NewLocation = FMath::Lerp(LookAtAttachStartCameraLocation, LookAtAttachTargetCameraLocation, Alpha);
	const FQuat NewRotation = LookAtAttachStartTransform.GetRotation();
	SpectatorPawn->SyncToTransform(FTransform(NewRotation, NewLocation, FVector::OneVector));

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		SpectatorPawn->SetCameraBoomLength(LookAtAttachTargetArmLength);
		SpectatorPawn->SyncToTransform(FTransform(NewRotation, LookAtAttachTargetCameraLocation, FVector::OneVector));
	SpectatorPawn->AttachToActor(TargetActor, FAttachmentTransformRules::KeepWorldTransform);
	if (USceneComponent* RootComponent = SpectatorPawn->GetRootComponent())
	{
		RootComponent->SetUsingAbsoluteRotation(true);
	}
	Possess(SpectatorPawn);

	if (const USpringArmComponent* LookAtArm = TargetActor->FindComponentByClass<USpringArmComponent>())
	{
		SpectatorPawn->SetCameraBoomLength(LookAtArm->TargetArmLength);
	}
		ClearLookAtAttachBlend();
	}
}

void APlayerMainController::ClearLookAtAttachBlend()
{
	GetWorldTimerManager().ClearTimer(LookAtAttachBlendHandle);
	bLookAtAttachBlendActive = false;
	LookAtAttachBlendElapsed = 0.0f;
	LookAtAttachTargetActor = nullptr;
}

FTransform APlayerMainController::GetPawnCameraTransform(APawn* Pawn) const
{
	if (!Pawn)
	{
		return FTransform::Identity;
	}

	if (const USpringArmComponent* Arm = Pawn->FindComponentByClass<USpringArmComponent>())
	{
		const FTransform CameraTransform = Arm->GetSocketTransform(USpringArmComponent::SocketName);
		return FTransform(CameraTransform.GetRotation(), CameraTransform.GetLocation(), FVector::OneVector);
	}

	if (const UCameraComponent* Camera = Pawn->FindComponentByClass<UCameraComponent>())
	{
		const FTransform CameraTransform = Camera->GetComponentTransform();
		return FTransform(CameraTransform.GetRotation(), CameraTransform.GetLocation(), FVector::OneVector);
	}

	const FTransform ActorTransform = Pawn->GetActorTransform();
	return FTransform(ActorTransform.GetRotation(), ActorTransform.GetLocation(), FVector::OneVector);
}

void APlayerMainController::BeginSwitchToPawnBlend(APawn* PawnToControl)
{
	APlayerSpectator* SpectatorPawn = CachedSpectatorPawn.Get();
	if (!SpectatorPawn)
	{
		SpectatorPawn = Cast<APlayerSpectator>(GetPawn());
		CachedSpectatorPawn = SpectatorPawn;
	}
	if (!SpectatorPawn || !PawnToControl)
	{
		return;
	}

	ClearSwitchToPawnBlend();

	if (SpectatorPawn->GetAttachParentActor())
	{
		SpectatorPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	if (USceneComponent* RootComponent = SpectatorPawn->GetRootComponent())
	{
		RootComponent->SetUsingAbsoluteRotation(false);
	}

	SwitchToPawnBlendTargetPawn = PawnToControl;
	SwitchToPawnBlendStartTransform = GetPawnCameraTransform(SpectatorPawn);
	SwitchToPawnBlendTargetTransform = GetPawnCameraTransform(PawnToControl);
	SwitchToPawnBlendElapsed = 0.0f;
	bSwitchToPawnBlendActive = true;
	SpectatorPawn->SyncToTransform(SwitchToPawnBlendStartTransform);

	if (LookAtAttachBlendDuration <= 0.0f)
	{
		UpdateSwitchToPawnBlend();
		return;
	}

	GetWorldTimerManager().SetTimer(SwitchToPawnBlendHandle, this, &APlayerMainController::UpdateSwitchToPawnBlend, 0.005f, true);
}

void APlayerMainController::UpdateSwitchToPawnBlend()
{
	if (!bSwitchToPawnBlendActive)
	{
		return;
	}

	APlayerSpectator* SpectatorPawn = CachedSpectatorPawn.Get();
	APawn* TargetPawn = SwitchToPawnBlendTargetPawn.Get();
	if (!SpectatorPawn || !TargetPawn)
	{
		ClearSwitchToPawnBlend();
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	SwitchToPawnBlendElapsed += DeltaSeconds;
	const float Duration = FMath::Max(LookAtAttachBlendDuration, 0.0f);
	const float Alpha = Duration > 0.0f
		? FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(SwitchToPawnBlendElapsed / Duration, 0.0f, 1.0f))
		: 1.0f;
	const FVector NewLocation = FMath::Lerp(SwitchToPawnBlendStartTransform.GetLocation(), SwitchToPawnBlendTargetTransform.GetLocation(), Alpha);
	const FQuat NewRotation = FQuat::Slerp(SwitchToPawnBlendStartTransform.GetRotation(), SwitchToPawnBlendTargetTransform.GetRotation(), Alpha);
	SpectatorPawn->SyncToTransform(FTransform(NewRotation, NewLocation, FVector::OneVector));

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		SpectatorPawn->SyncToTransform(SwitchToPawnBlendTargetTransform);
		ClearSwitchToPawnBlend();
		Possess(TargetPawn);
		if (TargetPawn->IsA<AShip>())
		{
			ClearCameraReset();
			CachedPitchInput = 0.0f;
			CachedYawInput = 0.0f;
			CachedRollInput = 0.0f;
			UpdateShipRotationInput();
			SetControlRotation(SwitchToPawnBlendTargetTransform.GetRotation().Rotator());
		}
	}
}

void APlayerMainController::ClearSwitchToPawnBlend()
{
	GetWorldTimerManager().ClearTimer(SwitchToPawnBlendHandle);
	bSwitchToPawnBlendActive = false;
	SwitchToPawnBlendElapsed = 0.0f;
	SwitchToPawnBlendTargetPawn = nullptr;
}

void APlayerMainController::BeginSpectatorRollReset(APlayerSpectator* SpectatorPawn)
{
	if (!SpectatorPawn)
	{
		return;
	}

	ClearSpectatorRollReset();
	SpectatorRollResetPawn = SpectatorPawn;
	SpectatorRollResetStartRotation = SpectatorPawn->GetActorRotation();
	SpectatorRollResetElapsed = 0.0f;

	if (LookAtAttachBlendDuration <= 0.0f)
	{
		UpdateSpectatorRollReset();
		return;
	}

	GetWorldTimerManager().SetTimer(SpectatorRollResetHandle, this, &APlayerMainController::UpdateSpectatorRollReset, 0.005f, true);
}

void APlayerMainController::UpdateSpectatorRollReset()
{
	APlayerSpectator* SpectatorPawn = SpectatorRollResetPawn.Get();
	if (!SpectatorPawn)
	{
		ClearSpectatorRollReset();
		return;
	}

	const float DeltaSeconds = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.0f;
	SpectatorRollResetElapsed += DeltaSeconds;
	const float Duration = FMath::Max(LookAtAttachBlendDuration, 0.0f);
	const float Alpha = Duration > 0.0f
		? FMath::SmoothStep(0.0f, 1.0f, FMath::Clamp(SpectatorRollResetElapsed / Duration, 0.0f, 1.0f))
		: 1.0f;
	FRotator NewRotation = SpectatorRollResetStartRotation;
	NewRotation.Roll = FMath::Lerp(SpectatorRollResetStartRotation.Roll, 0.0f, Alpha);
	SpectatorPawn->SyncToTransform(FTransform(NewRotation.Quaternion(), SpectatorPawn->GetActorLocation(), FVector::OneVector));

	if (Alpha >= 1.0f - KINDA_SMALL_NUMBER)
	{
		NewRotation.Roll = 0.0f;
		SpectatorPawn->SyncToTransform(FTransform(NewRotation.Quaternion(), SpectatorPawn->GetActorLocation(), FVector::OneVector));
		ClearSpectatorRollReset();
	}
}

void APlayerMainController::ClearSpectatorRollReset()
{
	GetWorldTimerManager().ClearTimer(SpectatorRollResetHandle);
	SpectatorRollResetElapsed = 0.0f;
	SpectatorRollResetPawn = nullptr;
}

void APlayerMainController::SwitchToPawn(APawn* PawnToControl)
{
    if (!PawnToControl)
    {
        return;
    }

	ClearSwitchToPawnBlend();
	ClearSpectatorRollReset();
	ClearLookAtAttachBlend();

    if (PawnToControl == GetPawn())
    {
        if (CachedSpectatorPawn.IsValid())
        {
			if (APawn* CurrentPawn = GetPawn())
			{
				const FTransform CameraTransform = GetPawnCameraTransform(CurrentPawn);
				CachedSpectatorPawn->SyncToTransform(CameraTransform);
			}
            CachedSpectatorPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
			if (USceneComponent* RootComponent = CachedSpectatorPawn->GetRootComponent())
			{
				RootComponent->SetUsingAbsoluteRotation(false);
			}
            Possess(CachedSpectatorPawn.Get());
			BeginSpectatorRollReset(CachedSpectatorPawn.Get());
        }
        return;
    }

    if (APlayerSpectator* SpectatorPawn = Cast<APlayerSpectator>(GetPawn()))
    {
        CachedSpectatorPawn = SpectatorPawn;
        BeginSwitchToPawnBlend(PawnToControl);
		return;
    }

    Possess(PawnToControl);
	if (PawnToControl->IsA<AShip>())
	{
		ClearCameraReset();
		CachedPitchInput = 0.0f;
		CachedYawInput = 0.0f;
		CachedRollInput = 0.0f;
		UpdateShipRotationInput();
		SetControlRotation(PawnToControl->GetActorRotation());
	}
}

void APlayerMainController::LookAtActor(AActor* LookAt)
{
	APlayerSpectator* SpectatorPawn = CachedSpectatorPawn.Get();
	if (!SpectatorPawn)
	{
		SpectatorPawn = Cast<APlayerSpectator>(GetPawn());
		CachedSpectatorPawn = SpectatorPawn;
	}
	if (!SpectatorPawn)
	{
		return;
	}

	if (bLookAtAttachBlendActive && LookAt == LookAtAttachTargetActor.Get())
	{
		ClearLookAtAttachBlend();
		return;
	}

	if (bLookAtAttachBlendActive && LookAt != LookAtAttachTargetActor.Get())
	{
		ClearLookAtAttachBlend();
	}

	if (LookAt && LookAt == SpectatorPawn->GetAttachParentActor())
	{
		const USpringArmComponent* SpectatorArm = SpectatorPawn->FindComponentByClass<USpringArmComponent>();
		const FTransform CameraTransform = SpectatorArm
			? SpectatorArm->GetSocketTransform(USpringArmComponent::SocketName)
			: SpectatorPawn->GetActorTransform();
		SpectatorPawn->SetCameraBoomLength(0.0f);
		SpectatorPawn->SyncToTransform(FTransform(CameraTransform.GetRotation(), CameraTransform.GetLocation(), FVector::OneVector));
		SpectatorPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		if (USceneComponent* RootComponent = SpectatorPawn->GetRootComponent())
		{
			RootComponent->SetUsingAbsoluteRotation(false);
		}
		Possess(SpectatorPawn);
		return;
	}

	if (LookAt)
	{
		BeginLookAtAttachBlend(LookAt);
		return;
	}

	if (AActor* AttachedActor = SpectatorPawn->GetAttachParentActor())
	{
		const USpringArmComponent* SpectatorArm = SpectatorPawn->FindComponentByClass<USpringArmComponent>();
		const FTransform CameraTransform = SpectatorArm
			? SpectatorArm->GetSocketTransform(USpringArmComponent::SocketName)
			: SpectatorPawn->GetActorTransform();
		SpectatorPawn->SetCameraBoomLength(0.0f);
		SpectatorPawn->SyncToTransform(FTransform(CameraTransform.GetRotation(), CameraTransform.GetLocation(), FVector::OneVector));
		SpectatorPawn->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		if (USceneComponent* RootComponent = SpectatorPawn->GetRootComponent())
		{
			RootComponent->SetUsingAbsoluteRotation(false);
		}
	}
}

