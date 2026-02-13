// Fill out your copyright notice in the Description page of Project Settings.

#include "PlayerSpectator.h"
#include "Camera/CameraComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputActionValue.h"

APlayerSpectator::APlayerSpectator()
{
	PrimaryActorTick.bCanEverTick = true;

	RootSphere = CreateDefaultSubobject<USphereComponent>(TEXT("RootSphere"));
	RootSphere->InitSphereRadius(32.0f);
	RootSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RootSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	RootComponent = RootSphere;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootSphere);
	CameraBoom->TargetArmLength = 0.0f;
	CameraBoom->bUsePawnControlRotation = true;

	SpectatorCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("SpectatorCamera"));
	SpectatorCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	SpectatorCamera->bUsePawnControlRotation = false;

	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->Acceleration = Acceleration;
	Movement->Deceleration = Deceleration;
	Movement->MaxSpeed = MaxSpeed;
	Movement->TurningBoost = 8.0f;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationRoll = false;
}

void APlayerSpectator::BeginPlay()
{
	Super::BeginPlay();

	if (Movement)
	{
		Movement->Acceleration = Acceleration;
		Movement->Deceleration = Deceleration;
		Movement->MaxSpeed = MaxSpeed;
	}

	TargetRotation = GetActorRotation();
}

void APlayerSpectator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator Smoothed = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, RotationInterpSpeed);
	SetActorRotation(Smoothed);
}

void APlayerSpectator::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
}

void APlayerSpectator::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void APlayerSpectator::ApplyMoveInput(const FInputActionValue& Value)
{
	const FVector MovementInput = Value.Get<FVector>();
	if (!Controller)
	{
		return;
	}

	const FVector Forward = GetActorForwardVector();
	const FVector Right = GetActorRightVector();
	const FVector Up = GetActorUpVector();

	AddMovementInput(Forward, MovementInput.X);
	AddMovementInput(Right, MovementInput.Y);
	AddMovementInput(Up, MovementInput.Z);
}

void APlayerSpectator::ApplyLookInput(const FInputActionValue& Value)
{
	if (!Controller)
	{
		return;
	}

	const FVector2D LookAxis = Value.Get<FVector2D>() * LookSensitivity;

	TargetRotation.Yaw += LookAxis.X;
	TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch + LookAxis.Y, PitchClampMin, PitchClampMax);
	TargetRotation.Roll = 0.0f;

	Controller->SetControlRotation(TargetRotation);
}

void APlayerSpectator::SetCameraBoomLength(float NewLength)
{
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = NewLength;
	}
}

float APlayerSpectator::GetCameraBoomLength() const
{
	return CameraBoom ? CameraBoom->TargetArmLength : 0.0f;
}

void APlayerSpectator::SyncToTransform(const FTransform& NewTransform)
{
	SetActorTransform(NewTransform);
	TargetRotation = NewTransform.GetRotation().Rotator();
	if (Controller)
	{
		Controller->SetControlRotation(TargetRotation);
	}
}