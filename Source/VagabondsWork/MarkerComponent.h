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

UENUM(BlueprintType)
enum class EFaction : uint8
{
	Neutral,
	Red,
	Blue
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VAGABONDSWORK_API UMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMarkerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marker")
	EMarkerType MarkerType = EMarkerType::Component;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EFaction Faction = EFaction::Neutral;
};