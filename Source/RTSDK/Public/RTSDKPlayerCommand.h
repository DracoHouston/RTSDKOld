// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "RTSDKConstants.h"
#include "FixedPointTypes.h"
#include "RTSDKPlayerCommand.generated.h"

USTRUCT()
struct RTSDK_API FRTSDKPlayerCommandReplicationInfo
{
	GENERATED_BODY()

		UPROPERTY()
		TSubclassOf<URTSDKPlayerCommandBase> Class;

	UPROPERTY()
		TArray<FFixedVector64> TargetLocations;

	UPROPERTY()
		TArray<FFixedRotator64> TargetRotations;

	UPROPERTY()
		TArray<int64> TargetUnitIDs;

	UPROPERTY()
		TArray<FName> TargetUnitTypes;
};

/**
* Abstract base class for "Player Commands" in RTSDK
* Created by the RTSDK Game Sim Subsystem when a confirmed command comes in.
* Players send commands via RPC, the arguments for which are:
* Target Locations - An array of vector values. Typically signify locations.
* Target Rotations - An array of rotator values. Typically signify rotations.
* Target Unit IDs - An array of integer values. Typically signify Unit IDs.
* Target Unit Types - An array of FName values. Typically signify Unit Types.
* These are available along with the Player that issued the command, as a PlayerState, as members.
* When the command is ready to execute the virtual function Execute is called.
* Commands must implement their own Execute method for the command to do anything.
*/
UCLASS(abstract)
class RTSDK_API URTSDKPlayerCommandBase : public UObject
{
	GENERATED_BODY()
public:
	virtual void Execute() {}

	void SetAll(APlayerState* inPlayer, const FRTSDKPlayerCommandReplicationInfo& Info)
	{
		TargetLocations = Info.TargetLocations;
		TargetRotations = Info.TargetRotations;
		TargetUnitIDs = Info.TargetUnitIDs;
		TargetUnitTypes = Info.TargetUnitTypes;
		Player = inPlayer;
	}

protected:
	TWeakObjectPtr<APlayerState> Player;
	TArray<FFixedVector64> TargetLocations;
	TArray<FFixedRotator64> TargetRotations;
	TArray<int64> TargetUnitIDs;
	TArray<FName> TargetUnitTypes;
};