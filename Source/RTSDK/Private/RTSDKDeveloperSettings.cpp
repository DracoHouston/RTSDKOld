// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKDeveloperSettings.h"
#include "RTSDKSimState.h"
#include "RTSDKTeamState.h"
#include "RTSDKForceState.h"
#include "RTSDKCommanderState.h"

URTSDKDeveloperSettings::URTSDKDeveloperSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	TargetSimFramesPerSecond = 32;
	MaxFramesPerGameThreadTick = 32;
	MinimumNetTurnDuration = 0.25;
	MinimumLANTurnDuration = 0.125;
	LockstepTimeoutTurnCount = 4;
	SimStateClass = ARTSDKSimStateServerClientLockstep::StaticClass();
	TeamStateClass = ARTSDKTeamStateServerClientLockstep::StaticClass();
	ForceStateClass = ARTSDKForceStateServerClientLockstep::StaticClass();
	CommanderStateClass = ARTSDKCommanderStateServerClientLockstep::StaticClass();
}