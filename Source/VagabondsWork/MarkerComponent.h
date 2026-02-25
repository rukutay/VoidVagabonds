#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MarkerComponent.generated.h"

UENUM(BlueprintType)
enum class EMarkerType : uint8
{
	Ship,
	Star,
	Planet,
	Component
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VAGABONDSWORK_API UMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMarkerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marker")
	EMarkerType MarkerType = EMarkerType::Component;
};