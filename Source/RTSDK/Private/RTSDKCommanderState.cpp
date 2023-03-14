// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKCommanderState.h"
#include "RTSDKTeamState.h"
#include "RTSDKForceState.h"
#include "RTSDKSimState.h"
#include "RTSDKGameSimSubsystem.h"
#include "Net/UnrealNetwork.h"

void FRTSDKTurnData::PreReplicatedRemove(const FRTSDKTurnDataArray& InArraySerializer)
{
}

void FRTSDKTurnData::PostReplicatedAdd(const FRTSDKTurnDataArray& InArraySerializer)
{
}

void FRTSDKTurnData::PostReplicatedChange(const FRTSDKTurnDataArray& InArraySerializer)
{
}

void FRTSDKTurnDataArray::AddTurn(FRTSDKTurnData inTurnData)
{
	int32 Idx = Turns.Add(inTurnData);
	MarkItemDirty(Turns[Idx]);

	// server calls "on rep" also
	Turns[Idx].PostReplicatedAdd(*this);
}

bool FRTSDKTurnDataArray::GetTurn(int32 inTurn, FRTSDKTurnData& outTurnData)
{
	for (int32 i = 0; i < Turns.Num(); i++)
	{
		if (Turns[i].Turn == inTurn)
		{
			outTurnData = Turns[i];
			return true;
		}
	}
	return false;
}

ARTSDKCommanderStateBase::ARTSDKCommanderStateBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void ARTSDKCommanderStateBase::Setup(const FRTSDKStateSetupInfo& inSetup, ARTSDKSimStateBase* inSimState, URTSDKGameSimSubsystem* inSimSubsystem)
{
	SimSubsystem = inSimSubsystem;
	SimState = inSimState;
	for (auto It = inSetup.OptionsMap.CreateConstIterator(); It; ++It)
	{
		if (It->Key == TEXT("Name"))
		{
			SetDisplayName(FText::FromString(It->Value));
		}
		else if (It->Key == TEXT("Force"))
		{
			int32 idx = FCString::Atoi(*It->Value);
			ARTSDKForceStateBase* force = SimState->GetForce(idx);
			if (force != nullptr)
			{
				SetForce(force);
			}
		}
	}
}

ARTSDKCommanderStateSPOnly::ARTSDKCommanderStateSPOnly(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = false;
}

FText ARTSDKCommanderStateSPOnly::GetDisplayName()
{
	return DisplayName;
}

void ARTSDKCommanderStateSPOnly::SetDisplayName(const FText& inName)
{
	DisplayName = inName;
}

TArray<FRTSDKPlayerCommandReplicationInfo> ARTSDKCommanderStateSPOnly::GetCommandsByTurn(int32 inTurn)
{
	FRTSDKTurnData turn;
	if (TurnData.GetTurn(inTurn, turn))
	{
		return turn.Commands;
	}
	return TArray<FRTSDKPlayerCommandReplicationInfo>();
}

void ARTSDKCommanderStateSPOnly::AddCommandToCommandBuffer(FRTSDKPlayerCommandReplicationInfo inCommand)
{
	InputCommandBuffer.Add(inCommand);
}

void ARTSDKCommanderStateSPOnly::FlushCommandBuffer()
{
	FRTSDKTurnData turn;
	turn.Commands = InputCommandBuffer;
	turn.Turn = SimSubsystem->GetCurrentInputTurn();
	TurnData.AddTurn(turn);
}

ARTSDKTeamStateBase* ARTSDKCommanderStateSPOnly::GetTeam()
{
	return Force->GetTeam();
}

ARTSDKForceStateBase* ARTSDKCommanderStateSPOnly::GetForce()
{
	return Force;
}

void ARTSDKCommanderStateSPOnly::SetForce(ARTSDKForceStateBase* inForce)
{
	Force = inForce;
}

ARTSDKCommanderStateServerClientLockstep::ARTSDKCommanderStateServerClientLockstep(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

FText ARTSDKCommanderStateServerClientLockstep::GetDisplayName()
{
	return DisplayName;
}

void ARTSDKCommanderStateServerClientLockstep::SetDisplayName(const FText& inName)
{
	DisplayName = inName;
}

TArray<FRTSDKPlayerCommandReplicationInfo> ARTSDKCommanderStateServerClientLockstep::GetCommandsByTurn(int32 inTurn)
{
	FRTSDKTurnData turn;
	if (TurnData.GetTurn(inTurn, turn))
	{
		return turn.Commands;
	}
	return TArray<FRTSDKPlayerCommandReplicationInfo>();
}

void ARTSDKCommanderStateServerClientLockstep::AddCommandToCommandBuffer(FRTSDKPlayerCommandReplicationInfo inCommand)
{
	InputCommandBuffer.Add(inCommand);
}

void ARTSDKCommanderStateServerClientLockstep::FlushCommandBuffer()
{
	FRTSDKTurnData turn;
	turn.Commands = InputCommandBuffer;
	turn.Turn = SimSubsystem->GetCurrentInputTurn();
	TurnData.AddTurn(turn);
}

ARTSDKTeamStateBase* ARTSDKCommanderStateServerClientLockstep::GetTeam()
{
	return Force->GetTeam();
}

ARTSDKForceStateBase* ARTSDKCommanderStateServerClientLockstep::GetForce()
{
	return Force;
}

void ARTSDKCommanderStateServerClientLockstep::SetForce(ARTSDKForceStateBase* inForce)
{
	Force = inForce;
}

void ARTSDKCommanderStateServerClientLockstep::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARTSDKCommanderStateServerClientLockstep, TurnData);
	DOREPLIFETIME(ARTSDKCommanderStateServerClientLockstep, Force);
	DOREPLIFETIME(ARTSDKCommanderStateServerClientLockstep, DisplayName);
}

ARTSDKCommanderStateServerClientCurves::ARTSDKCommanderStateServerClientCurves(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;
	bAlwaysRelevant = true;
}

FText ARTSDKCommanderStateServerClientCurves::GetDisplayName()
{
	return DisplayName;
}

void ARTSDKCommanderStateServerClientCurves::SetDisplayName(const FText& inName)
{
	DisplayName = inName;
}

TArray<FRTSDKPlayerCommandReplicationInfo> ARTSDKCommanderStateServerClientCurves::GetCommandsByTurn(int32 inTurn)
{
	FRTSDKTurnData turn;
	if (TurnData.GetTurn(inTurn, turn))
	{
		return turn.Commands;
	}
	return TArray<FRTSDKPlayerCommandReplicationInfo>();
}

void ARTSDKCommanderStateServerClientCurves::AddCommandToCommandBuffer(FRTSDKPlayerCommandReplicationInfo inCommand)
{
	InputCommandBuffer.Add(inCommand);
}

void ARTSDKCommanderStateServerClientCurves::FlushCommandBuffer()
{
	FRTSDKTurnData turn;
	turn.Commands = InputCommandBuffer;
	turn.Turn = SimSubsystem->GetCurrentInputTurn();
	TurnData.AddTurn(turn);
}

ARTSDKTeamStateBase* ARTSDKCommanderStateServerClientCurves::GetTeam()
{
	return Force->GetTeam();
}

ARTSDKForceStateBase* ARTSDKCommanderStateServerClientCurves::GetForce()
{
	return Force;
}

void ARTSDKCommanderStateServerClientCurves::SetForce(ARTSDKForceStateBase* inForce)
{
	Force = inForce;
}

void ARTSDKCommanderStateServerClientCurves::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARTSDKCommanderStateServerClientCurves, Force);
	DOREPLIFETIME(ARTSDKCommanderStateServerClientCurves, DisplayName);
}
