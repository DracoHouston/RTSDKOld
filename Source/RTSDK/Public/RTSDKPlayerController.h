// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FixedPointTypes.h"
#include "RTSDKPlayerCommand.h"
#include "RTSDKPlayerController.generated.h"

class URTSDKPlayerCommandBase;

/**
 * 
 */
UCLASS()
class RTSDK_API ARTSDKPlayerController : public APlayerController
{
	GENERATED_BODY()

		UFUNCTION(Server, Reliable)
		void Server_SendPlayerCommand(const FRTSDKPlayerCommandReplicationInfo& inCommand);

	UFUNCTION(Server, Reliable)
		void Server_PlayerIsReady(bool bIsReady);
	
};
