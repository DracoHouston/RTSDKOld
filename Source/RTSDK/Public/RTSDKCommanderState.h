// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "RTSDKPlayerCommand.h"
#include "RTSDKCommanderState.generated.h"

class URTSDKGameSimSubsystem;
class ARTSDKSimStateBase;
class ARTSDKForceStateBase;
class ARTSDKTeamStateBase;

struct FRTSDKTurnDataArray;

USTRUCT()
struct RTSDK_API FRTSDKTurnData : public FFastArraySerializerItem
{
	GENERATED_BODY()

		UPROPERTY()
		int32 Turn;

	UPROPERTY()
		TArray<FRTSDKPlayerCommandReplicationInfo> Commands;

	void PreReplicatedRemove(const FRTSDKTurnDataArray& InArraySerializer);
	void PostReplicatedAdd(const FRTSDKTurnDataArray& InArraySerializer);
	void PostReplicatedChange(const FRTSDKTurnDataArray& InArraySerializer);

};

USTRUCT()
struct RTSDK_API FRTSDKTurnDataArray : public FFastArraySerializer
{
	GENERATED_BODY()

		void AddTurn(FRTSDKTurnData inTurnData);

	bool GetTurn(int32 inTurn, FRTSDKTurnData& outTurnData);

	UPROPERTY()
		TArray<FRTSDKTurnData> Turns;

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FRTSDKTurnData, FRTSDKTurnDataArray>(Turns, DeltaParms, *this);
	}
};

/** Specified to allow fast TArray replication */
template<>
struct TStructOpsTypeTraits<FRTSDKTurnDataArray> : public TStructOpsTypeTraitsBase2<FRTSDKTurnDataArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

/**
 * Abstract base class for the state of a Commander.
 * Commanders are the will behind forces.
 * Can be a player, ai, or replay dummy.
 * Child classes of this implement different networking environments.
 */
UCLASS(abstract)
class RTSDK_API ARTSDKCommanderStateBase : public AInfo
{
	GENERATED_BODY()

public:

	ARTSDKCommanderStateBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION()
		virtual void Setup(const FRTSDKStateSetupInfo& inSetup, ARTSDKSimStateBase* inSimState, URTSDKGameSimSubsystem* inSimSubsystem);

	UFUNCTION()
		virtual FText GetDisplayName() { return FText(); }

	UFUNCTION()
		virtual void SetDisplayName(const FText& inName) {}

	//Get all commands for this commander for a turn as an array of player command infos
	UFUNCTION()
	virtual TArray<FRTSDKPlayerCommandReplicationInfo> GetCommandsByTurn(int32 inTurn) { return TArray<FRTSDKPlayerCommandReplicationInfo>();	}

	//Add a command to the command buffer, as a player command info
	UFUNCTION()
	virtual void AddCommandToCommandBuffer(FRTSDKPlayerCommandReplicationInfo inCommand) {}

	//Flush the current command buffer, adding it to the commands per turn for the current input turn.
	UFUNCTION()
	virtual void FlushCommandBuffer() {}

	//Get the team this commander belongs to.
	UFUNCTION()
	virtual ARTSDKTeamStateBase* GetTeam() { return nullptr; }

	//Get the force this commander belongs to.
	UFUNCTION()
		virtual ARTSDKForceStateBase* GetForce() { return nullptr; }

	UFUNCTION()
		virtual void SetForce(ARTSDKForceStateBase* inForce) { }

protected:

	//Set on spawn, the sim subsystem this commander belongs to.
	UPROPERTY(Transient)
	TObjectPtr<URTSDKGameSimSubsystem> SimSubsystem;

	//Set on spawn, the sim state this commander belongs to.
	UPROPERTY(Transient)
	TObjectPtr<ARTSDKSimStateBase> SimState;

	//Input Command Buffer, the commands so far for this input turn. 
	//FlushCommandBuffer to commit to turn data.
	UPROPERTY(Transient)
	TArray<FRTSDKPlayerCommandReplicationInfo> InputCommandBuffer;
};

/**
 * RTSDK Commander State SP Only, the singleplayer only version of RTSDK Commander State Base.
 * No networking.
 */
UCLASS()
class RTSDK_API ARTSDKCommanderStateSPOnly : public ARTSDKCommanderStateBase
{
	GENERATED_BODY()

public:

	ARTSDKCommanderStateSPOnly(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual FText GetDisplayName() override;
	virtual void SetDisplayName(const FText& inName) override;
	virtual TArray<FRTSDKPlayerCommandReplicationInfo> GetCommandsByTurn(int32 inTurn) override;
	virtual void AddCommandToCommandBuffer(FRTSDKPlayerCommandReplicationInfo inCommand) override;
	virtual void FlushCommandBuffer() override;
	virtual ARTSDKTeamStateBase* GetTeam() override;
	virtual ARTSDKForceStateBase* GetForce() override;
	virtual void SetForce(ARTSDKForceStateBase* inForce) override;

protected:
	
	UPROPERTY(Transient)
		TObjectPtr<ARTSDKForceStateBase> Force;

	UPROPERTY(Transient)
		FRTSDKTurnDataArray TurnData;

	UPROPERTY(Transient)
		FText DisplayName;
};

/**
 * RTSDK Commander State SP, the singleplayer only version of RTSDK Commander State Base.
 * Networked via actor replication. All players get turn data.
 */
UCLASS()
class RTSDK_API ARTSDKCommanderStateServerClientLockstep : public ARTSDKCommanderStateBase
{
	GENERATED_BODY()

public:

	ARTSDKCommanderStateServerClientLockstep(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual FText GetDisplayName() override;
	virtual void SetDisplayName(const FText& inName) override;
	virtual TArray<FRTSDKPlayerCommandReplicationInfo> GetCommandsByTurn(int32 inTurn) override;
	virtual void AddCommandToCommandBuffer(FRTSDKPlayerCommandReplicationInfo inCommand) override;
	virtual void FlushCommandBuffer() override;
	virtual ARTSDKTeamStateBase* GetTeam() override;
	virtual ARTSDKForceStateBase* GetForce() override;
	virtual void SetForce(ARTSDKForceStateBase* inForce) override;

protected:

	UPROPERTY(Transient, Replicated)
	TObjectPtr<ARTSDKForceStateBase> Force;

	UPROPERTY(Transient, Replicated)
	FRTSDKTurnDataArray TurnData;

	UPROPERTY(Transient, Replicated)
		FText DisplayName;
};

/**
 * RTSDK Commander State Server Client Curves, a multiplayer version of RTSDK Commander State Base.
 * Networked via actor replication. Only server sees turn data.
 */
UCLASS()
class RTSDK_API ARTSDKCommanderStateServerClientCurves : public ARTSDKCommanderStateBase
{
	GENERATED_BODY()

public:

	ARTSDKCommanderStateServerClientCurves(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual FText GetDisplayName() override;
	virtual void SetDisplayName(const FText& inName) override;
	virtual TArray<FRTSDKPlayerCommandReplicationInfo> GetCommandsByTurn(int32 inTurn) override;
	virtual void AddCommandToCommandBuffer(FRTSDKPlayerCommandReplicationInfo inCommand) override;
	virtual void FlushCommandBuffer() override;
	virtual ARTSDKTeamStateBase* GetTeam() override;
	virtual ARTSDKForceStateBase* GetForce() override;
	virtual void SetForce(ARTSDKForceStateBase* inForce) override;

protected:

	UPROPERTY(Transient, Replicated)
		TObjectPtr<ARTSDKForceStateBase> Force;

	UPROPERTY(Transient)
		FRTSDKTurnDataArray TurnData;

	UPROPERTY(Transient, Replicated)
		FText DisplayName;
};