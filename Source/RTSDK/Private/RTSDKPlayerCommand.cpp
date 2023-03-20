// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKPlayerCommand.h"
#include "RTSDKCommanderState.h"
#include "RTSDKTeamState.h"
#include "RTSDKForceState.h"
#include "RTSDKSimState.h"
#include "RTSDKGameSimSubsystem.h"

void URTSDKPlayerCommandBase::SetAll(ARTSDKCommanderStateBase* inCommander, URTSDKGameSimSubsystem* inSimSubsystem, const FRTSDKPlayerCommandReplicationInfo& Info)
{
	TargetLocations = Info.TargetLocations;
	TargetRotations = Info.TargetRotations;
	TargetUnitIDs = Info.TargetUnitIDs;
	UnitIDs = Info.UnitIDs;
	TargetUnitTypes = Info.TargetUnitTypes;
	Commander = inCommander;
	Force = Commander->GetForce();
	Team = Force->GetTeam();
	SimSubsystem = inSimSubsystem;
	SimState = SimSubsystem->GetSimState();
}

void URTSDKPausePlayerCommand::Execute()
{
	SimState->SetMatchIsPaused(true);
}

void URTSDKUnpausePlayerCommand::Execute()
{
	SimState->SetMatchIsPaused(false);
}
