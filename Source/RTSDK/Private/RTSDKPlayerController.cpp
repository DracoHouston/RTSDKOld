// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKPlayerController.h"
#include "RTSDKPlayerState.h"
#include "FixedPointTypes.h"
#include "RTSDKPlayerCommand.h"
#include "RTSDKCommanderState.h"
#include "RTSDKGameSimSubsystem.h"
#include "RTSDKSimState.h"
#include "Net/UnrealNetwork.h"

ARTSDKPlayerController::ARTSDKPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bWantsToBeReady = false;
}

void ARTSDKPlayerController::BeginPlay()
{
	Super::BeginPlay();
#if WITH_EDITORONLY_DATA
	UWorld* world = GetWorld();
	if ((world->WorldType == EWorldType::PIE) && IsLocalPlayerController())
	{
		SetReadyState(true);
	}
#endif
}

void ARTSDKPlayerController::Server_SendPlayerCommand_Implementation(const FRTSDKPlayerCommandReplicationInfo& inCommand)
{
	SendPlayerCommand(inCommand);
}

void ARTSDKPlayerController::Server_SetReadyState_Implementation(bool inIsReady)
{
	SetReadyState(inIsReady);
}

void ARTSDKPlayerController::OnRep_CommanderState()
{
	if (bWantsToBeReady)
	{
		SetReadyState(true);
	}
}

void ARTSDKPlayerController::RTSDKTogglePause()
{
	UWorld* world = GetWorld();
	if (world == nullptr)
	{
		return;
	}
	URTSDKGameSimSubsystem* sim = world->GetSubsystem<URTSDKGameSimSubsystem>();
	if (sim == nullptr)
	{
		return;
	}
	ARTSDKSimStateBase* simstate = sim->GetSimState();
	if (simstate == nullptr)
	{
		return;
	}
	if (simstate->GetMatchIsPaused())
	{
		RequestUnpause();
		return;
	}
	RequestPause();
}

void ARTSDKPlayerController::RTSDKRequestPause()
{
	RequestPause();
}

void ARTSDKPlayerController::RTSDKRequestUnpause()
{
	RequestUnpause();
}

void ARTSDKPlayerController::Server_RequestPause_Implementation()
{
	RequestPause();
}

void ARTSDKPlayerController::Server_RequestUnpause_Implementation()
{
	RequestUnpause();
}

void ARTSDKPlayerController::SetReadyState(bool inIsReady)
{
	if (CommanderState == nullptr)
	{
		bWantsToBeReady = inIsReady;
		return;
	}
	if (GetLocalRole() != ENetRole::ROLE_Authority)
	{
		Server_SetReadyState(inIsReady);
		return;
	}
	CommanderState->SetIsReady(inIsReady);
}

void ARTSDKPlayerController::SendPlayerCommand(const FRTSDKPlayerCommandReplicationInfo& inCommand)
{
	if (CommanderState == nullptr)
	{
		return;
	}
	if (GetLocalRole() != ENetRole::ROLE_Authority)
	{
		Server_SendPlayerCommand_Implementation(inCommand);
		return;
	}
	CommanderState->AddCommandToCommandBuffer(inCommand);
}

void ARTSDKPlayerController::FinishInputTurn(int32 inTurn, int32 inChecksum)
{
	if (CommanderState == nullptr)
	{
		return;
	}
	if (GetLocalRole() != ENetRole::ROLE_Authority)
	{
		Server_FinishInputTurn(inTurn, inChecksum);
		return;
	}
	CommanderState->SetLastCompletedTurn(inTurn);
}

void ARTSDKPlayerController::RequestPause()
{
	if (GetLocalRole() != ENetRole::ROLE_Authority)
	{
		Server_RequestPause();
		return;
	}

	UWorld* world = GetWorld();
	if (world == nullptr)
	{
		return;
	}
	URTSDKGameSimSubsystem* sim = world->GetSubsystem<URTSDKGameSimSubsystem>();
	if (sim == nullptr)
	{
		return;
	}
	ARTSDKSimStateBase* simstate = sim->GetSimState();
	if (simstate == nullptr)
	{
		return;
	}

	simstate->RequestPause(this);
}

void ARTSDKPlayerController::RequestUnpause()
{
	if (GetLocalRole() != ENetRole::ROLE_Authority)
	{
		Server_RequestUnpause();
		return;
	}

	UWorld* world = GetWorld();
	if (world == nullptr)
	{
		return;
	}
	URTSDKGameSimSubsystem* sim = world->GetSubsystem<URTSDKGameSimSubsystem>();
	if (sim == nullptr)
	{
		return;
	}
	ARTSDKSimStateBase* simstate = sim->GetSimState();
	if (simstate == nullptr)
	{
		return;
	}

	simstate->RequestUnpause(this);
}

ARTSDKCommanderStateBase* ARTSDKPlayerController::GetCommanderState()
{
	return CommanderState;
}

void ARTSDKPlayerController::SetCommanderState(ARTSDKCommanderStateBase* inState)
{
	if (GetLocalRole() == ENetRole::ROLE_Authority)
	{
		CommanderState = inState;
		OnRep_CommanderState();
	}
}

bool ARTSDKPlayerController::GetWantsToBeReady()
{
	return bWantsToBeReady;
}

void ARTSDKPlayerController::Server_FinishInputTurn_Implementation(int32 inTurn, int32 inChecksum)
{
	FinishInputTurn(inTurn, inChecksum);
}

void ARTSDKPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARTSDKPlayerController, CommanderState);
}