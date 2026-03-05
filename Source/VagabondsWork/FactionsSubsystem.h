#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FactionsSubsystem.generated.h"

UENUM(BlueprintType)
enum class EFaction : uint8
{
	VoidVagabonds = 0 UMETA(DisplayName = "Void Vagabonds"),
	VoidRaiders = 1 UMETA(DisplayName = "Void Raiders"),
	SilentMandate = 2 UMETA(DisplayName = "Silent Mandate"),
	FederationOfNations = 3 UMETA(DisplayName = "Federation of Nations"),
	UnitedRepublic = 4 UMETA(DisplayName = "United Republic"),
	ConcordUnion = 5 UMETA(DisplayName = "Concord Union"),
	None = 6 UMETA(DisplayName = "None")
};

UCLASS()
class VAGABONDSWORK_API UFactionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 FactionCount = 7;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Faction|Relations")
	int32 GetRelation(EFaction A, EFaction B) const;

	UFUNCTION(BlueprintCallable, Category = "Faction|Relations")
	void SetRelation(EFaction A, EFaction B, int32 Value);

	UFUNCTION(BlueprintCallable, Category = "Faction|Relations")
	void UpdateRelations(EFaction A, EFaction B, float Modifier);

	UFUNCTION(BlueprintCallable, Category = "Faction|Relations")
	void ResetDefaults();

	static FORCEINLINE int32 ToIndex(EFaction F)
	{
		return FMath::Clamp(static_cast<int32>(F), 0, FactionCount - 1);
	}

	static FORCEINLINE int32 FlatIndex(int32 A, int32 B)
	{
		return (A * FactionCount) + B;
	}

	static FORCEINLINE int8 ClampRelation(int32 Value)
	{
		return static_cast<int8>(FMath::Clamp(Value, -50, 50));
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Faction")
	EFaction DefaultFaction = EFaction::VoidVagabonds;

	UFUNCTION(BlueprintPure, Category = "Faction")
	EFaction GetDefaultFaction() const
	{
		return DefaultFaction;
	}

private:
	int8 RelationFlat[FactionCount * FactionCount];
};