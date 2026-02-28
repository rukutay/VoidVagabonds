#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MarkerComponent.generated.h"

UENUM(BlueprintType)
enum class EMarkerType : uint8
{
	Ship UMETA(DisplayName = "Ship"),
	Star UMETA(DisplayName = "Star"),
	Planet UMETA(DisplayName = "Planet"),
	Component UMETA(DisplayName = "Component")
};

UENUM(BlueprintType)
enum class EFaction : uint8
{
	VoidVagabonds UMETA(DisplayName = "Void Vagabonds"),
	VoidRaiders UMETA(DisplayName = "Void Raiders"),
	SilentMandate UMETA(DisplayName = "Silent Mandate"),
	FederationOfNations UMETA(DisplayName = "Federation of Nations"),
	UnitedRepublic UMETA(DisplayName = "United Republic"),
	ConcordUnion UMETA(DisplayName = "Concord Union")
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
	EFaction Faction = EFaction::VoidVagabonds;
};