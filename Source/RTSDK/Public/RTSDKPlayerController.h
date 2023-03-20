// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "RTSDKPlayerControllerInterface.h"
#include "RTSDKPlayerController.generated.h"

class URTSDKPlayerCommandBase;
class ARTSDKCommanderStateBase;

/**
 * 
 */
UCLASS()
class RTSDK_API ARTSDKPlayerController : public APlayerController, public IRTSDKPlayerControllerInterface
{
	GENERATED_BODY()
public:

	ARTSDKPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//UObject Interface
	virtual void BeginPlay() override;
	//~UObject Interface
	// 
	//RTSDKPlayerControllerInterface
	virtual void SetReadyState(bool inIsReady) override;
	virtual void SendPlayerCommand(const FRTSDKPlayerCommandReplicationInfo& inCommand) override;
	virtual void FinishInputTurn(int32 inTurn, int32 inChecksum) override;
	virtual void RequestPause() override;
	virtual void RequestUnpause() override;
	virtual ARTSDKCommanderStateBase* GetCommanderState() override;
	virtual void SetCommanderState(ARTSDKCommanderStateBase* inState) override;
	virtual bool GetWantsToBeReady() override;
	//~RTSDKPlayerControllerInterface

	UFUNCTION()
	virtual void OnRep_CommanderState();

		UFUNCTION(Server, Reliable)
		void Server_SendPlayerCommand(const FRTSDKPlayerCommandReplicationInfo& inCommand);

	UFUNCTION(Server, Reliable)
		void Server_SetReadyState(bool inIsReady);

	UFUNCTION(Server, Reliable)
		void Server_FinishInputTurn(int32 inTurn, int32 inChecksum);

	UFUNCTION(Server, Reliable)
		void Server_RequestPause();

	UFUNCTION(Server, Reliable)
		void Server_RequestUnpause();	

	UFUNCTION(Exec)
		void RTSDKTogglePause();

	UFUNCTION(Exec)
		void RTSDKRequestPause();

	UFUNCTION(Exec)
		void RTSDKRequestUnpause();

protected:

	UPROPERTY(Transient, ReplicatedUsing = OnRep_CommanderState)
		TObjectPtr<ARTSDKCommanderStateBase> CommanderState;

	bool bWantsToBeReady;
	
};
