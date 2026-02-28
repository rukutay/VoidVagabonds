#include "FactionsSubsystem.h"

void UFactionsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	ResetDefaults();
}

int32 UFactionsSubsystem::GetRelation(EFaction A, EFaction B) const
{
	const int32 IndexA = ToIndex(A);
	const int32 IndexB = ToIndex(B);
	return static_cast<int32>(RelationFlat[FlatIndex(IndexA, IndexB)]);
}

void UFactionsSubsystem::SetRelation(EFaction A, EFaction B, int32 Value, bool bSymmetric)
{
	const int32 IndexA = ToIndex(A);
	const int32 IndexB = ToIndex(B);

	int8 ClampedValue = ClampRelation(Value);
	if (IndexA == IndexB)
	{
		ClampedValue = 0;
	}

	RelationFlat[FlatIndex(IndexA, IndexB)] = ClampedValue;
	if (bSymmetric)
	{
		RelationFlat[FlatIndex(IndexB, IndexA)] = ClampedValue;
	}
}

void UFactionsSubsystem::ResetDefaults()
{
	for (int32 Index = 0; Index < (FactionCount * FactionCount); ++Index)
	{
		RelationFlat[Index] = 0;
	}

	const EFaction Raiders = EFaction::VoidRaiders;
	for (int32 FactionIndex = 0; FactionIndex < FactionCount; ++FactionIndex)
	{
		const EFaction OtherFaction = static_cast<EFaction>(FactionIndex);
		if (OtherFaction == Raiders)
		{
			continue;
		}

		SetRelation(Raiders, OtherFaction, -50, true);
	}

	for (int32 FactionIndex = 0; FactionIndex < FactionCount; ++FactionIndex)
	{
		RelationFlat[FlatIndex(FactionIndex, FactionIndex)] = 0;
	}
}