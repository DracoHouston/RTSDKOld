// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSGameSimProcessorBase.h"
#include "RTSConstants.h"
#include "RTSGameSimSubsystem.h"

bool URTSGameSimProcessorBase::TryAdvanceFrame()
{
	UWorld* world = GetWorld();
	if (world == nullptr)
	{
		return false;
	}
	URTSGameSimSubsystem* sim = world->GetSubsystem<URTSGameSimSubsystem>();
	if (sim == nullptr)
	{
		return false;
	}
	if (!sim->IsSimRunning())
	{
		return false;
	}
	if (sim->GetFrameCount() == LastProcessedFrame)
	{
		return false;
	}
	LastProcessedFrame = sim->GetFrameCount();
	return true;
}
