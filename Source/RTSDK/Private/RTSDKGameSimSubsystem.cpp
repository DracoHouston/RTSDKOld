// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKGameSimSubsystem.h"
#include "RTSDKMassModuleSettings.h"
#include "MassExecutor.h"
#include "MassEntitySubsystem.h"
#include "MassSignalSubsystem.h"
#include "RTSDKPlayerState.h"
#include "RTSDKGameStateBase.h"
#include "RTSDKSimState.h"
#include "RTSDKTeamState.h"
#include "RTSDKForceState.h"
#include "RTSDKCommanderState.h"
#include "RTSDKWorldSettings.h"
#include "Engine/World.h"

void URTSDKGameSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	bInScriptCallingMode = false;
	PlayerCommandsByTurn.Empty();
	ResetFrameCount();
	ResetUnits();
	//SetFrameDelay(1);
	SetTimeScale(1.0);
	SetMaxFramesPerTick(20);
	SetTargetUPS(32);
	SetFramesPerLockstepTurn(TargetUPS / 4);
	SetMetersToUUScale(100.0);
	SetGravityDirection(FVector::DownVector);
	SetGravityAcceleration(9.8);
	SetTerminalVelocity(40.0);
	Super::Initialize(Collection);
}

void URTSDKGameSimSubsystem::PostInitialize()
{
	Super::PostInitialize();
}

void URTSDKGameSimSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	FString DependencyGraphFileName;
	const URTSDKMassModuleSettings* Settings = GetMutableDefault<URTSDKMassModuleSettings>();
#if WITH_EDITOR
	
	if (Settings != nullptr && !Settings->DumpDependencyGraphFileName.IsEmpty())
	{
		DependencyGraphFileName = FString::Printf(TEXT("%s_%s"), *Settings->DumpDependencyGraphFileName, *ToString(InWorld.GetNetMode()));
	}
#endif // WITH_EDITOR
	FString FileName = !DependencyGraphFileName.IsEmpty() ? FString::Printf(TEXT("%s_%s"), *DependencyGraphFileName, *Settings->SimProcessingPhaseConfig.PhaseName.ToString()) : FString();
	UMassEntitySubsystem* EntitySubsystem = InWorld.GetSubsystem<UMassEntitySubsystem>();
	EntityManager = EntitySubsystem->GetMutableEntityManager().AsShared();
	SimProcessor = NewObject<UMassCompositeProcessor>(this, Settings->SimProcessingPhaseConfig.PhaseGroupClass,
		*FString::Printf(TEXT("ProcessingPhase_%s"), *Settings->SimProcessingPhaseConfig.PhaseName.ToString()));
	SimProcessor->CopyAndSort(Settings->SimProcessingPhaseConfig, FileName);
	SimProcessor->SetProcessingPhase(EMassProcessingPhase::PrePhysics);
	SimProcessor->SetGroupName(FName(FString::Printf(TEXT("%s Group"), *Settings->SimProcessingPhaseConfig.PhaseName.ToString())));
	SimProcessor->Initialize(*this);

	LastRealTimeSeconds = InWorld.RealTimeSeconds;
	bSimIsPaused = false;
	bSimIsRunning = true;
	if (InWorld.GetNetMode() != NM_Client)
	{
		ARTSDKSimStateBase* simstate = InWorld.SpawnActor<ARTSDKSimStateBase>(ARTSDKSimStateServerClientLockstep::StaticClass());
		simstate->Setup(this, &InWorld);
	}

}

void URTSDKGameSimSubsystem::Deinitialize()
{
	PlayerCommandsByTurn.Empty();
	UnitsByID.Empty();
	bSimIsRunning = false;
	bInScriptCallingMode = false;
	ScriptCommandBuffer.Reset();
	Super::Deinitialize();
}

TStatId URTSDKGameSimSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URTSDKGameSimSubsystem, STATGROUP_Tickables);
}

void URTSDKGameSimSubsystem::Tick(float DeltaTime)
{
	if (!bSimIsRunning)
	{
		return;
	}
	FRTSNumber64 realtimeseconds = GetWorld()->RealTimeSeconds;
	FRTSNumber64 deltarealtime = realtimeseconds - LastRealTimeSeconds;
	if (!bSimIsPaused)
	{
		PausedTimeSeconds += deltarealtime;
		PausedDilatedTimeSeconds += (deltarealtime * TimeScale);
		if (ShouldFinalizeLockstepTurn())
		{
			FinalizeLockstepTurn();
		}
		int32 frames = 0;
		while (ShouldAdvanceFrame() && (frames < MaxFramesPerTick))
		{
			AdvanceFrame();
			frames++;
		}
	}
	
	LastRealTimeSeconds = realtimeseconds;
}

void URTSDKGameSimSubsystem::AdvanceFrame()
{
	FrameCount++;
	GameTimeSeconds = (FRTSNumber64)FrameCount * TimeStep;
	ScriptCommandBuffer.Reset();
	ScriptCommandBuffer = MakeShareable(new FMassCommandBuffer());
	UMassProcessor& proc = *SimProcessor;
	FMassProcessingContext Context(EntityManager, TimeStep);
	Context.bFlushCommandBuffer = true;
	UE::Mass::Executor::Run(proc, Context);
	EntityManager->FlushCommands(ScriptCommandBuffer);
}

bool URTSDKGameSimSubsystem::ShouldAdvanceFrame() const
{
	UWorld* world = GetWorld();
	if (world == nullptr)
	{
		return false;
	}

	return FRTSMath::FloorToNumber64(PausedDilatedTimeSeconds / TimeStep) > (FRTSNumber64)FrameCount;
}

TArray<TObjectPtr<URTSDKPlayerCommandBase>> URTSDKGameSimSubsystem::GetPlayerCommandsByTurn(int32 inTurn) const
{
	const TArray<TObjectPtr<URTSDKPlayerCommandBase>>* retval = nullptr;
	retval = PlayerCommandsByTurn.Find(inTurn);
	return *retval;
}

void URTSDKGameSimSubsystem::AddPlayerCommands(APlayerState* inPlayer, const TArray<FRTSDKPlayerCommandReplicationInfo>& inCommandInputs)
{
	TArray<TObjectPtr<URTSDKPlayerCommandBase>>& turncommands = PlayerCommandsByTurn.FindOrAdd(CurrentInputTurn + 1);
	for (int32 i = 0; i < inCommandInputs.Num(); i++)
	{
		URTSDKPlayerCommandBase* command = NewObject<URTSDKPlayerCommandBase>(this, inCommandInputs[i].Class.Get());
		command->SetAll(inPlayer, inCommandInputs[i]);
		turncommands.Add(command);
	}
}

int64 URTSDKGameSimSubsystem::RegisterUnit(AActor* inUnitActor, URTSDKUnitComponent* inUnitComponent, FMassEntityHandle inUnitHandle)
{
	if ((inUnitActor == nullptr) || (!inUnitHandle.IsValid()))
	{
		return -1;
	}

	int64 newid = ClaimUnitID();
	FRTSDKRegisteredUnitInfo& newunit = UnitsByID.Add(newid);
	newunit.UnitActor = inUnitActor;
	newunit.UnitComponent = inUnitComponent;
	newunit.UnitHandle = inUnitHandle;
	return newid;
}

FMassCommandBuffer& URTSDKGameSimSubsystem::StartScriptCallingMode()
{
	bInScriptCallingMode = true;
	return *ScriptCommandBuffer.Get();
}

void URTSDKGameSimSubsystem::EndScriptCallingMode()
{
	bInScriptCallingMode = false;
}

void URTSDKGameSimSubsystem::SetGlobalGravityDirection(FVector inDir)
{
	UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(GetWorld());
	inDir.Normalize();
	SetGravityDirection(inDir);
	SignalSubsystem->SignalEntities(UE::Mass::Signals::RTSDKUnitGravityChanged, GetAllUnitEntityHandles());
}

bool URTSDKGameSimSubsystem::ShouldFinalizeLockstepTurn()
{
	if ((LastLockstepTurnFrame + FramesPerLockstepTurn) <= FrameCount)
	{
		return true;
	}
	return false;
}

struct FLockstepFinalizeInfo
{
	APlayerState* player;
	int32 index; 
	TArray<FRTSDKPlayerCommandReplicationInfo> commands;

	FLockstepFinalizeInfo(APlayerState* p, int32 i, TArray<FRTSDKPlayerCommandReplicationInfo> c)
	{
		player = p;
		index = i;
		commands = c;
	}

	inline bool operator<(const FLockstepFinalizeInfo& Other) const
	{
		return index < Other.index;
	}
};

void URTSDKGameSimSubsystem::FinalizeLockstepTurn()
{
	/*bool isserver = GetWorld()->IsServer();
	TArray<TObjectPtr<APlayerState>> players = GetWorld()->GetGameState()->PlayerArray;
	TArray<FLockstepFinalizeInfo> infos;
	int32 playercount = 0;
	for (int32 i = 0; i < players.Num(); i++)
	{
		ARTSDKPlayerState* rtsdkplayer = Cast<ARTSDKPlayerState>(players[i]);
		if ((rtsdkplayer == nullptr) || (rtsdkplayer->IsSpectator()))
		{
			continue;
		}
		playercount++;
		if (isserver)
		{
			rtsdkplayer->AddCurrentFrameToFrameData(CurrentInputTurn);
		}
		for (int32 t = 0; t < rtsdkplayer->TurnData.Turns.Num(); t++)
		{
			if (rtsdkplayer->TurnData.Turns[t].Turn == CurrentInputTurn - 1)
			{
				infos.Add(FLockstepFinalizeInfo(rtsdkplayer, rtsdkplayer->PlayerIndex, rtsdkplayer->TurnData.Turns[t].Commands));
			}
		}
	}
	if (infos.Num() != playercount)
	{
		return;
	}*/
	CurrentInputTurn++;
	LastLockstepTurnFrame = FrameCount;
	
	/*infos.Sort();*/
	/*for (int32 i = 0; i < infos.Num(); i++)
	{
		AddPlayerCommands(infos[i].player, infos[i].commands);
	}*/
}
