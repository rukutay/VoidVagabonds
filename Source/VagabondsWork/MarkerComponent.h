#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FactionsSubsystem.h"
#include "MarkerComponent.generated.h"

UENUM(BlueprintType)
enum class EMarkerType : uint8
{
	Ship UMETA(DisplayName = "Ship"),
	Star UMETA(DisplayName = "Star"),
	Planet UMETA(DisplayName = "Planet"),
	Component UMETA(DisplayName = "Component")
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VAGABONDSWORK_API UMarkerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMarkerComponent();
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Marker")
	EMarkerType MarkerType = EMarkerType::Component;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EFaction Faction;
};