// Copyright Epic Games, Inc. All Rights Reserved.

#include "VagabondsWorkGameMode.h"
#include "PlayerSpectator.h"

AVagabondsWorkGameMode::AVagabondsWorkGameMode()
{
	DefaultPawnClass = APlayerSpectator::StaticClass();
}