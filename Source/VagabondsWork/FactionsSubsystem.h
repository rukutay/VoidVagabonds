#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "FactionsSubsystem.generated.h"

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

UCLASS()
class VAGABONDSWORK_API UFactionsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 FactionCount = 6;

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "Faction|Relations")
	int32 GetRelation(EFaction A, EFaction B) const;

	UFUNCTION(BlueprintCallable, Category = "Faction|Relations")
	void SetRelation(EFaction A, EFaction B, int32 Value, bool bSymmetric = true);

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