// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TimerManager.h"
#include "ShipVitalityComponent.generated.h"

// Forward declarations
class AActor;
enum class EShipPreset : uint8;

UENUM(BlueprintType)
enum class EDamageType : uint8
{
    DT_Unknown      UMETA(DisplayName = "Unknown"),
    DT_Kinetic      UMETA(DisplayName = "Kinetic"),
    DT_Energy       UMETA(DisplayName = "Energy"),
    DT_Explosive    UMETA(DisplayName = "Explosive"),
    DT_Heat         UMETA(DisplayName = "Heat")
};

USTRUCT(BlueprintType)
struct FShipDamageSpec
{
    GENERATED_BODY()

    /** Amount of damage to apply */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Amount = 0.0f;

    /** Whether this damage bypasses shields */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bBypassShield = false;

    /** Armor penetration factor (0..1) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ArmorPenetration = 0.0f;

    /** Type of damage */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDamageType Type = EDamageType::DT_Unknown;

    /** Actor that caused this damage */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    AActor* Instigator = nullptr;

    /** Location where the hit occurred */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector HitLocation = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FShipDamageResult
{
    GENERATED_BODY()

    /** Change in shield health */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ShieldDelta = 0.0f;

    /** Change in hull health */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float HullDelta = 0.0f;

    /** Final hull health value */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FinalHull = 0.0f;

    /** Final shield health value */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FinalShield = 0.0f;

    /** Whether the ship was destroyed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDestroyed = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageTaken, const FShipDamageResult&, DamageResult);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDestroyed, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldChanged, float, NewValue, float, MaxValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHullChanged, float, NewValue, float, MaxValue);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VAGABONDSWORK_API UShipVitalityComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UShipVitalityComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
    /** Time when last damage was applied */
    float LastDamageTime = 0.0f;

    /** Handle for the shield recharge delay timer */
    FTimerHandle ShieldRechargeDelayHandle;

    /** Handle for the shield recharge tick timer */
    FTimerHandle ShieldRechargeTickHandle;

    /** Start the shield recharge delay timer */
    void StartShieldRechargeDelay();

    /** Begin the shield recharge process */
    void BeginShieldRecharge();

    /** Tick the shield recharge process */
    void TickShieldRecharge();

public:
    /**
     * Apply damage to the ship
     * @param DamageSpec Specification of the damage to apply
     * @return Result of the damage application
     */
    UFUNCTION(BlueprintCallable, Category = "Ship|Vitality")
    FShipDamageResult ApplyDamage(const FShipDamageSpec& DamageSpec);

    UFUNCTION(BlueprintCallable, Category = "Ship|Vitality|Preset")
    void ApplyVitalityPreset(EShipPreset Preset);

    /** Maximum hull health points */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality|Hull")
    float HullHPMax = 150.0f;

    /** Current hull health points */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality|Hull")
    float HullHP = 150.0f;

    /** Maximum shield health points */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality|Shield")
    float ShieldHPMax = 350.0f;

    /** Current shield health points */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality|Shield")
    float ShieldHP = 350.0f;

    /** Rate at which shields recharge per second */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality|Shield", meta = (ClampMin = "0.0"))
    float ShieldRechargeRate = 25.0f;

    /** Delay before shield recharge begins after taking damage (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality|Shield", meta = (ClampMin = "0.0"))
    float ShieldRechargeDelay = 5.0f;

    /** Interval between shield recharge ticks (seconds) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality|Shield", meta = (ClampMin = "0.0"))
    float ShieldRechargeTickInterval = 0.25f;

    /** Armor damage reduction factor (0..1, where 1 means full reduction) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality|Armor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ArmorDamageReduction = 0.0f;

    /** Whether the ship has been destroyed */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ship|Vitality")
    bool bDestroyed = false;

    /** Delegate called when damage is taken */
    UPROPERTY(BlueprintAssignable, Category = "Ship|Vitality|Events")
    FOnDamageTaken OnDamageTaken;

    /** Delegate called when the ship is destroyed */
    UPROPERTY(BlueprintAssignable, Category = "Ship|Vitality|Events")
    FOnDestroyed OnDestroyed;

    /** Delegate called when shield health changes */
    UPROPERTY(BlueprintAssignable, Category = "Ship|Vitality|Events")
    FOnShieldChanged OnShieldChanged;

    /** Delegate called when hull health changes */
    UPROPERTY(BlueprintAssignable, Category = "Ship|Vitality|Events")
    FOnHullChanged OnHullChanged;
};