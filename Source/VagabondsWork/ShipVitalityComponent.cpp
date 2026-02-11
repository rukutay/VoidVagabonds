// Fill out your copyright notice in the Description page of Project Settings.

#include "ShipVitalityComponent.h"
#include "Ship.h"

// Sets default values for this component's properties
UShipVitalityComponent::UShipVitalityComponent()
{
	// Set this component to be initialized when the game starts, and to NOT be ticked every frame
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UShipVitalityComponent::BeginPlay()
{
	Super::BeginPlay();
}

FShipDamageResult UShipVitalityComponent::ApplyDamage(const FShipDamageSpec& DamageSpec)
{
    FShipDamageResult Result;
    
    // Early out if already destroyed or no damage
    if (bDestroyed || DamageSpec.Amount <= 0.0f)
    {
        Result.FinalHull = HullHP;
        Result.FinalShield = ShieldHP;
        Result.bDestroyed = bDestroyed;
        return Result;
    }
    
    float RemainingDamage = DamageSpec.Amount;
    float InitialShield = ShieldHP;
    float InitialHull = HullHP;
    
    // Apply shield damage if not bypassed
    if (!DamageSpec.bBypassShield && ShieldHP > 0.0f)
    {
        float ShieldAbsorbed = FMath::Min(ShieldHP, RemainingDamage);
        ShieldHP -= ShieldAbsorbed;
        RemainingDamage -= ShieldAbsorbed;
        
        // Clamp shield to valid range
        ShieldHP = FMath::Clamp(ShieldHP, 0.0f, ShieldHPMax);
    }
    
    // Apply armor damage reduction if there's remaining damage
    if (RemainingDamage > 0.0f && ArmorDamageReduction > 0.0f)
    {
        RemainingDamage *= (1.0f - ArmorDamageReduction);
    }
    
    // Apply hull damage
    if (RemainingDamage > 0.0f)
    {
        HullHP -= RemainingDamage;
        
        // Clamp hull to valid range
        HullHP = FMath::Clamp(HullHP, 0.0f, HullHPMax);
    }
    
    // Update last damage time
    LastDamageTime = GetWorld()->GetTimeSeconds();
    
    // Stop any active shield recharge
    GetWorld()->GetTimerManager().ClearTimer(ShieldRechargeTickHandle);
    GetWorld()->GetTimerManager().ClearTimer(ShieldRechargeDelayHandle);
    
    // Calculate deltas
    Result.ShieldDelta = ShieldHP - InitialShield;
    Result.HullDelta = HullHP - InitialHull;
    Result.FinalHull = HullHP;
    Result.FinalShield = ShieldHP;
    
    // Start shield recharge delay if any damage was applied
    if (Result.ShieldDelta != 0.0f || Result.HullDelta != 0.0f)
    {
        StartShieldRechargeDelay();
    }
    
    // Check for destruction
    if (HullHP <= 0.0f)
    {
        bDestroyed = true;
        Result.bDestroyed = true;
        OnDestroyed.Broadcast(DamageSpec.Instigator);
    }
    
    // Broadcast events
    if (Result.ShieldDelta != 0.0f)
    {
        OnShieldChanged.Broadcast(ShieldHP, ShieldHPMax);
    }
    
    if (Result.HullDelta != 0.0f)
    {
        OnHullChanged.Broadcast(HullHP, HullHPMax);
    }
    
    // Always broadcast damage taken (even if no actual damage occurred)
    Result.bDestroyed = bDestroyed;
    OnDamageTaken.Broadcast(Result);
    
    return Result;
}

void UShipVitalityComponent::ApplyVitalityPreset(EShipPreset Preset)
{
    float NewHullHPMax = HullHPMax;
    float NewShieldHPMax = ShieldHPMax;
    float NewShieldRechargeRate = ShieldRechargeRate;
    float NewShieldRechargeDelay = ShieldRechargeDelay;
    float NewShieldRechargeTickInterval = ShieldRechargeTickInterval;
    float NewArmorDamageReduction = ArmorDamageReduction;

    switch (Preset)
    {
        case EShipPreset::Fighter:
        {
            NewHullHPMax = 150.0f;
            NewShieldHPMax = 350.0f;
            NewShieldRechargeRate = 25.0f;
            NewShieldRechargeDelay = 5.0f;
            NewShieldRechargeTickInterval = 0.25f;
            NewArmorDamageReduction = 0.0f;
            break;
        }
        case EShipPreset::Interceptor:
        {
            NewHullHPMax = 120.0f;
            NewShieldHPMax = 280.0f;
            NewShieldRechargeRate = 30.0f;
            NewShieldRechargeDelay = 4.5f;
            NewShieldRechargeTickInterval = 0.25f;
            NewArmorDamageReduction = 0.05f;
            break;
        }
        case EShipPreset::Gunship:
        {
            NewHullHPMax = 220.0f;
            NewShieldHPMax = 450.0f;
            NewShieldRechargeRate = 22.0f;
            NewShieldRechargeDelay = 5.5f;
            NewShieldRechargeTickInterval = 0.3f;
            NewArmorDamageReduction = 0.1f;
            break;
        }
        case EShipPreset::Cruiser:
        {
            NewHullHPMax = 320.0f;
            NewShieldHPMax = 600.0f;
            NewShieldRechargeRate = 18.0f;
            NewShieldRechargeDelay = 6.0f;
            NewShieldRechargeTickInterval = 0.35f;
            NewArmorDamageReduction = 0.2f;
            break;
        }
        case EShipPreset::Carrier:
        {
            NewHullHPMax = 450.0f;
            NewShieldHPMax = 800.0f;
            NewShieldRechargeRate = 15.0f;
            NewShieldRechargeDelay = 6.5f;
            NewShieldRechargeTickInterval = 0.4f;
            NewArmorDamageReduction = 0.3f;
            break;
        }
        default:
            break;
    }

    HullHPMax = NewHullHPMax;
    ShieldHPMax = NewShieldHPMax;
    ShieldRechargeRate = NewShieldRechargeRate;
    ShieldRechargeDelay = NewShieldRechargeDelay;
    ShieldRechargeTickInterval = NewShieldRechargeTickInterval;
    ArmorDamageReduction = NewArmorDamageReduction;

    HullHP = HullHPMax;
    ShieldHP = ShieldHPMax;
    bDestroyed = false;
}

void UShipVitalityComponent::StartShieldRechargeDelay()
{
    if (!bDestroyed && ShieldRechargeRate > 0.0f && ShieldHP < ShieldHPMax)
    {
        GetWorld()->GetTimerManager().SetTimer(ShieldRechargeDelayHandle, this, &UShipVitalityComponent::BeginShieldRecharge, ShieldRechargeDelay, false);
    }
}

void UShipVitalityComponent::BeginShieldRecharge()
{
    if (bDestroyed || ShieldHP >= ShieldHPMax || ShieldRechargeRate <= 0.0f)
    {
        return;
    }
    
    GetWorld()->GetTimerManager().SetTimer(ShieldRechargeTickHandle, this, &UShipVitalityComponent::TickShieldRecharge, ShieldRechargeTickInterval, true);
}

void UShipVitalityComponent::TickShieldRecharge()
{
    if (bDestroyed || ShieldHP >= ShieldHPMax || ShieldRechargeRate <= 0.0f)
    {
        GetWorld()->GetTimerManager().ClearTimer(ShieldRechargeTickHandle);
        return;
    }
    
    float RechargeAmount = ShieldRechargeRate * ShieldRechargeTickInterval;
    ShieldHP = FMath::Min(ShieldHPMax, ShieldHP + RechargeAmount);
    
    OnShieldChanged.Broadcast(ShieldHP, ShieldHPMax);
    
    if (ShieldHP >= ShieldHPMax)
    {
        ShieldHP = ShieldHPMax;
        GetWorld()->GetTimerManager().ClearTimer(ShieldRechargeTickHandle);
    }
}