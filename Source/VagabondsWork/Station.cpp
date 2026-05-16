// Fill out your copyright notice in the Description page of Project Settings.

#include "Station.h"

#include "MarkerComponent.h"

AStation::AStation()
{
	if (MarkerComponent)
	{
		MarkerComponent->MarkerType = EMarkerType::Station;
	}
}