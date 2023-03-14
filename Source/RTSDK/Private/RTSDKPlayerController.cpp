// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKPlayerController.h"
#include "RTSDKPlayerState.h"

void ARTSDKPlayerController::Server_SendPlayerCommand_Implementation(const FRTSDKPlayerCommandReplicationInfo& inCommand)
{
	/*if (PlayerState == nullptr)
	{
		return;
	}
	ARTSDKPlayerState* rtsdkplayerstate = Cast<ARTSDKPlayerState>(PlayerState);
	if (rtsdkplayerstate == nullptr)
	{
		return;
	}
	rtsdkplayerstate->InputCommandBuffer.Add(inCommand);*/
}

void ARTSDKPlayerController::Server_PlayerIsReady_Implementation(bool bIsReady)
{
	if (PlayerState == nullptr)
	{
		return;
	}
	ARTSDKPlayerState* rtsdkplayerstate = Cast<ARTSDKPlayerState>(PlayerState);
	if (rtsdkplayerstate == nullptr)
	{
		return;
	}
	rtsdkplayerstate->bIsReady = bIsReady;
}
