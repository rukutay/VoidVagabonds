#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AIShipController.generated.h"

class AShip;

UCLASS()
class VAGABONDSWORK_API AAIShipController : public AAIController
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    FVector GetFocusLocation();

    UFUNCTION(BlueprintCallable, Category="Rotation")
    void ApplyShipRotation(FVector TargetLocation);

private:
};
