// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavStaticBig.h"
#include "Station.generated.h"

UENUM(BlueprintType)
enum class EStationGoodsType : uint8
{
	Ore UMETA(DisplayName = "Ore"),
	Gas UMETA(DisplayName = "Gas"),
	Metals UMETA(DisplayName = "Metals"),
	Fuel UMETA(DisplayName = "Fuel"),
	Parts UMETA(DisplayName = "Parts"),
	Food UMETA(DisplayName = "Food"),
	Medicine UMETA(DisplayName = "Medicine"),
	ConsumerGoods UMETA(DisplayName = "Consumer Goods"),
	Electronics UMETA(DisplayName = "Electronics"),
	Ammunition UMETA(DisplayName = "Ammunition")
};

UENUM(BlueprintType)
enum class EStationArchetype : uint8
{
	None UMETA(DisplayName = "None"),
	MiningStation UMETA(DisplayName = "Mining Station"),
	RefineryStation UMETA(DisplayName = "Refinery Station"),
	IndustrialStation UMETA(DisplayName = "Industrial Station"),
	TradeHub UMETA(DisplayName = "Trade Hub"),
	MilitaryOutpost UMETA(DisplayName = "Military Outpost"),
	PirateBase UMETA(DisplayName = "Pirate Base")
};

USTRUCT(BlueprintType)
struct FStationGoodsEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|Economy")
	EStationGoodsType GoodsType = EStationGoodsType::Ore;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|Economy")
	int32 Amount = 0;
};

UCLASS()
class VAGABONDSWORK_API AStation : public ANavStaticBig
{
	GENERATED_BODY()

public:
	AStation();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|Identity")
	EStationArchetype StationArchetype = EStationArchetype::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|Identity")
	int32 SecurityLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|Identity")
	int32 TradeImportance = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|Economy")
	TArray<FStationGoodsEntry> SupplyGoods;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Station|Economy")
	TArray<FStationGoodsEntry> DemandGoods;
};