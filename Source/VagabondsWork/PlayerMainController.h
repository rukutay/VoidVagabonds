// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PlayerMainController.generated.h"

class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UMapWidget;
class AActor;
class AShip;

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

	UFUNCTION(BlueprintCallable, Category = "Input")
	void LookAtActor(AActor* LookAt);

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
	void HandleToggleCursorInput(const FInputActionValue& Value);
	void HandleLookOverrideInput(const FInputActionValue& Value);
	void UpdateShipRotationInput();
	void UpdateShipMappingContext(APawn* InPawn);
	void ScheduleCameraReset();
	void BeginCameraReset();
	void UpdateCameraReset();
	void ClearCameraReset();
	void BeginLookAtAttachBlend(AActor* LookAt);
	void UpdateLookAtAttachBlend();
	void ClearLookAtAttachBlend();
	void BeginSwitchToPawnBlend(APawn* PawnToControl);
	void UpdateSwitchToPawnBlend();
	void ClearSwitchToPawnBlend();
	void BeginSpectatorRollReset(class APlayerSpectator* SpectatorPawn);
	void UpdateSpectatorRollReset();
	void ClearSpectatorRollReset();
	void BeginShipCameraFollow();
	void UpdateShipCameraFollow();
	void ClearShipCameraFollow();
	FTransform GetPawnCameraTransform(APawn* Pawn) const;
	bool IsLookInputAllowed() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float CameraResetDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	float LookAtAttachBlendDuration = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	TWeakObjectPtr<AShip> LastPossessedShip;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interface")
	AActor* FocusActor;

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

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* ToggleCursorAction = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* LookOverrideAction = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<class APlayerSpectator> CachedSpectatorPawn;


	FTimerHandle CameraResetDelayHandle;
	FTimerHandle CameraResetInterpHandle;
	bool bCameraResetActive = false;
	FTimerHandle LookAtAttachBlendHandle;
	FTransform LookAtAttachStartTransform;
	FTransform LookAtAttachTargetTransform;
	FVector LookAtAttachStartCameraLocation = FVector::ZeroVector;
	FVector LookAtAttachTargetCameraLocation = FVector::ZeroVector;
	float LookAtAttachStartArmLength = 0.0f;
	float LookAtAttachTargetArmLength = 0.0f;
	TWeakObjectPtr<AActor> LookAtAttachTargetActor;
	float LookAtAttachBlendElapsed = 0.0f;
	bool bLookAtAttachBlendActive = false;
	bool bLookAtAttachBlendIsAttach = false;
	FTimerHandle SwitchToPawnBlendHandle;
	FTransform SwitchToPawnBlendStartTransform;
	FTransform SwitchToPawnBlendTargetTransform;
	float SwitchToPawnBlendElapsed = 0.0f;
	bool bSwitchToPawnBlendActive = false;
	TWeakObjectPtr<APawn> SwitchToPawnBlendTargetPawn;
	FTimerHandle SpectatorRollResetHandle;
	float SpectatorRollResetElapsed = 0.0f;
	FRotator SpectatorRollResetStartRotation = FRotator::ZeroRotator;
	TWeakObjectPtr<APlayerSpectator> SpectatorRollResetPawn;
	FTimerHandle ShipCameraFollowHandle;

	float CachedPitchInput = 0.f;
	float CachedYawInput = 0.f;
	float CachedRollInput = 0.f;
	bool bCursorVisibleMode = true;
	bool bLookOverrideHeld = false;
};
