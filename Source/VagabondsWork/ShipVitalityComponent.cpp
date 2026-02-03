// Fill out your copyright notice in the Description page of Project Settings.

#include "ShipVitalityComponent.h"

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