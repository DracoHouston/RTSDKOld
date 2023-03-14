// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "RTSDKPlayerCommand.h"
#include "RTSDKSimState.generated.h"

class ARTSDKCommanderStateBase;
class ARTSDKForceStateBase;
class ARTSDKTeamStateBase;
class URTSDKGameSimSubsystem;

USTRUCT()
struct RTSDK_API FRTSDKStateSetupInfo
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TMap<FString, FString> OptionsMap;

	FRTSDKStateSetupInfo() {}
	FRTSDKStateSetupInfo(TMap<FString, FString>& inOptions) : OptionsMap(inOptions) {}
};

/**
 * Info actor for the RTSDK Game Sim Subsystem.
 * Abstract class, child classes handle different network environments
 */
UCLASS(abstract)
class RTSDK_API ARTSDKSimStateBase : public AInfo
{
	GENERATED_BODY()

public:

	ARTSDKSimStateBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION()
	virtual void Setup(URTSDKGameSimSubsystem* inSimSubsystem, UWorld* inWorld);

	UFUNCTION()
		virtual void SetupTeams(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) {}

	UFUNCTION()
		virtual void SetupForces(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) {}

	UFUNCTION()
		virtual void SetupCommanders(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) {}

	UFUNCTION()
	virtual ARTSDKCommanderStateBase* GetCommander(const int32& inCommanderID) { return nullptr; }

	UFUNCTION()
	virtual TArray<ARTSDKCommanderStateBase*> GetCommanders() { return TArray<ARTSDKCommanderStateBase*>(); }

	UFUNCTION()
		virtual int32 AddCommander(ARTSDKCommanderStateBase* inCommanderState) { return -1; }

	UFUNCTION()
		virtual void SetCommanders(TArray<ARTSDKCommanderStateBase*> inCommanderStates) {}

	UFUNCTION()
		virtual int32 GetCommanderCount() { return 0; }

	UFUNCTION()
		virtual ARTSDKForceStateBase* GetForce(const int32& inForceID) { return nullptr; }
	
	UFUNCTION()
		virtual TArray<ARTSDKForceStateBase*> GetForces() { return TArray<ARTSDKForceStateBase*>(); }

	UFUNCTION()
		virtual int32 AddForce(ARTSDKForceStateBase* inForceState) { return -1; }

	UFUNCTION()
		virtual void SetForces(TArray<ARTSDKForceStateBase*> inForceStates) {}

	UFUNCTION()
		virtual int32 GetForceCount() { return 0; }

	UFUNCTION()
		virtual ARTSDKTeamStateBase* GetTeam(const int32& inTeamID) { return nullptr; }

	UFUNCTION()
		virtual TArray<ARTSDKTeamStateBase*> GetTeams() { return TArray<ARTSDKTeamStateBase*>(); }

	UFUNCTION()
		virtual int32 AddTeam(ARTSDKTeamStateBase* inTeamState) { return -1; }

	UFUNCTION()
		virtual void SetTeams(TArray<ARTSDKTeamStateBase*> inTeamStates) {}

	UFUNCTION()
		virtual int32 GetTeamCount() { return 0; }

	UFUNCTION()
	virtual TArray<FRTSDKPlayerCommandReplicationInfo> GetCommandsForCommanderByTurn(const int32& inCommanderID, const int32& inTurn) 
	{ 
		return TArray<FRTSDKPlayerCommandReplicationInfo>(); 
	}

protected:

	UPROPERTY(Transient)
	TObjectPtr<URTSDKGameSimSubsystem> SimSubsystem;

	UPROPERTY(Transient)
		FString OptionsString;
};

/**
 * RTSDK Sim State SP Only, the singleplayer only version of RTSDK Sim State Base.
 * No networking. Command turns happen every frame.
 */
UCLASS()
class RTSDK_API ARTSDKSimStateSPOnly : public ARTSDKSimStateBase
{
	GENERATED_BODY()

public:

	ARTSDKSimStateSPOnly(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//RTSDKSimState interface
	virtual void SetupTeams(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual void SetupForces(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual void SetupCommanders(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual ARTSDKCommanderStateBase* GetCommander(const int32& inCommanderID) override;
	virtual TArray<ARTSDKCommanderStateBase*> GetCommanders() override;
	virtual int32 AddCommander(ARTSDKCommanderStateBase* inCommanderState) override;
	virtual void SetCommanders(TArray<ARTSDKCommanderStateBase*> inCommanderStates) override;
	virtual int32 GetCommanderCount() override;
	virtual ARTSDKForceStateBase* GetForce(const int32& inForceID) override;
	virtual TArray<ARTSDKForceStateBase*> GetForces() override;
	virtual int32 AddForce(ARTSDKForceStateBase* inForceState) override;
	virtual void SetForces(TArray<ARTSDKForceStateBase*> inForceStates) override;
	virtual int32 GetForceCount() override;
	virtual ARTSDKTeamStateBase* GetTeam(const int32& inTeamID) override;
	virtual TArray<ARTSDKTeamStateBase*> GetTeams() override;
	virtual int32 AddTeam(ARTSDKTeamStateBase* inTeamState) override;
	virtual void SetTeams(TArray<ARTSDKTeamStateBase*> inTeamStates) override;
	virtual int32 GetTeamCount() override;
	virtual TArray<FRTSDKPlayerCommandReplicationInfo> GetCommandsForCommanderByTurn(const int32& inCommanderID, const int32& inTurn) override;
	//~RTSDKSimState interface

protected:

	UPROPERTY(Transient)
		TArray<TObjectPtr<ARTSDKCommanderStateBase>> Commanders;

	UPROPERTY(Transient)
		TArray<TObjectPtr<ARTSDKForceStateBase>> Forces;

	UPROPERTY(Transient)
		TArray<TObjectPtr<ARTSDKTeamStateBase>> Teams;
};

/**
 * RTSDK Sim State Server Client Lockstep, a multiplayer version of RTSDKSimStateBase
 * Performs lockstep networking via Unreal's built in server-client replication.
 * This networking method is useful for games that wish to run dedicated servers
 * that can record replays. Sim runs on all machines. Command turns frame duration is
 * dependent on highest client ping. Commands replicate to all clients, commands are sent to the server.
 */
UCLASS()
class RTSDK_API ARTSDKSimStateServerClientLockstep : public ARTSDKSimStateBase
{
	GENERATED_BODY()

public:

	ARTSDKSimStateServerClientLockstep(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//RTSDKSimState interface
	virtual void SetupTeams(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual void SetupForces(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual void SetupCommanders(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual ARTSDKCommanderStateBase* GetCommander(const int32& inCommanderID) override;
	virtual TArray<ARTSDKCommanderStateBase*> GetCommanders() override;
	virtual int32 AddCommander(ARTSDKCommanderStateBase* inCommanderState) override;
	virtual void SetCommanders(TArray<ARTSDKCommanderStateBase*> inCommanderStates) override;
	virtual int32 GetCommanderCount() override;
	virtual ARTSDKForceStateBase* GetForce(const int32& inForceID) override;
	virtual TArray<ARTSDKForceStateBase*> GetForces() override;
	virtual int32 AddForce(ARTSDKForceStateBase* inForceState) override;
	virtual void SetForces(TArray<ARTSDKForceStateBase*> inForceStates) override;
	virtual int32 GetForceCount() override;
	virtual ARTSDKTeamStateBase* GetTeam(const int32& inTeamID) override;
	virtual TArray<ARTSDKTeamStateBase*> GetTeams() override;
	virtual int32 AddTeam(ARTSDKTeamStateBase* inTeamState) override;
	virtual void SetTeams(TArray<ARTSDKTeamStateBase*> inTeamStates) override;
	virtual int32 GetTeamCount() override;
	virtual TArray<FRTSDKPlayerCommandReplicationInfo> GetCommandsForCommanderByTurn(const int32& inCommanderID, const int32& inTurn) override;
	//~RTSDKSimState interface

protected:

	UPROPERTY(Transient, Replicated)
		TArray<TObjectPtr<ARTSDKCommanderStateBase>> Commanders;

	UPROPERTY(Transient, Replicated)
		TArray<TObjectPtr<ARTSDKForceStateBase>> Forces;

	UPROPERTY(Transient, Replicated)
		TArray<TObjectPtr<ARTSDKTeamStateBase>> Teams;
};

/**
 * RTSDK Sim State Server Client Curves, a multiplayer version of RTSDKSimStateBase
 * Performs keyframed curve based networking via Unreal's built in server-client replication.
 * This networking method is useful for games that wish to have smooth direct player control
 * of units.
 * Curve replication sends keyframes for all relevant unit's properties to clients, the sim runs
 * only on the server. For most properties, values can be interpolated between keyframes. 
 * Keyframes are predicted. Command turns happen every sim frame. Commands are not replicated back 
 * to clients, only sent to server.
 */
UCLASS()
class RTSDK_API ARTSDKSimStateServerClientCurves : public ARTSDKSimStateBase
{
	GENERATED_BODY()

public:

	ARTSDKSimStateServerClientCurves(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//RTSDKSimState interface
	virtual void SetupTeams(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual void SetupForces(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual void SetupCommanders(TMap<int32, FRTSDKStateSetupInfo>& OptionsMap) override;
	virtual ARTSDKCommanderStateBase* GetCommander(const int32& inCommanderID) override;
	virtual TArray<ARTSDKCommanderStateBase*> GetCommanders() override;
	virtual int32 AddCommander(ARTSDKCommanderStateBase* inCommanderState) override;
	virtual void SetCommanders(TArray<ARTSDKCommanderStateBase*> inCommanderStates) override;
	virtual int32 GetCommanderCount() override;
	virtual ARTSDKForceStateBase* GetForce(const int32& inForceID) override;
	virtual TArray<ARTSDKForceStateBase*> GetForces() override;
	virtual int32 AddForce(ARTSDKForceStateBase* inForceState) override;
	virtual void SetForces(TArray<ARTSDKForceStateBase*> inForceStates) override;
	virtual int32 GetForceCount() override;
	virtual ARTSDKTeamStateBase* GetTeam(const int32& inTeamID) override;
	virtual TArray<ARTSDKTeamStateBase*> GetTeams() override;
	virtual int32 AddTeam(ARTSDKTeamStateBase* inTeamState) override;
	virtual void SetTeams(TArray<ARTSDKTeamStateBase*> inTeamStates) override;
	virtual int32 GetTeamCount() override;
	virtual TArray<FRTSDKPlayerCommandReplicationInfo> GetCommandsForCommanderByTurn(const int32& inCommanderID, const int32& inTurn) override;
	//~RTSDKSimState interface

protected:

	UPROPERTY(Transient, Replicated)
		TArray<TObjectPtr<ARTSDKCommanderStateBase>> Commanders;

	UPROPERTY(Transient, Replicated)
		TArray<TObjectPtr<ARTSDKForceStateBase>> Forces;

	UPROPERTY(Transient, Replicated)
		TArray<TObjectPtr<ARTSDKTeamStateBase>> Teams;
};
