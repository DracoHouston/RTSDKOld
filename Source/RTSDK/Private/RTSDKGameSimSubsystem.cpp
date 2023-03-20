// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKGameSimSubsystem.h"
#include "RTSDKMassModuleSettings.h"
#include "RTSDKDeveloperSettings.h"
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

	const URTSDKDeveloperSettings* RTSDKSettings = GetDefault<URTSDKDeveloperSettings>();
	SetTimeScale(1.0);
	SetMaxFramesPerTick(RTSDKSettings->MaxFramesPerGameThreadTick);
	SetTargetUPS(RTSDKSettings->TargetSimFramesPerSecond);
	
	SetMetersToUUScale(InWorld.GetWorldSettings()->WorldToMeters);
	SetGravityDirection(FVector::DownVector);
	SetGravityAcceleration(9.8);
	SetTerminalVelocity(40.0);

	LastRealTimeSeconds = InWorld.RealTimeSeconds;
	bSimIsPaused = false;
	if (InWorld.GetNetMode() != NM_Client)
	{
		ARTSDKSimStateBase* simstate = InWorld.SpawnActor<ARTSDKSimStateBase>(RTSDKSettings->SimStateClass);
		simstate->Setup(this, &InWorld);
		SimState = simstate;
		bSimIsRunning = true;
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
	if (!SimState->GetMatchHasStarted())
	{
		ERTSDKPreMatchTickResult prematchresult = SimState->PreMatchTick();
		if (prematchresult == ERTSDKPreMatchTickResult::Ready)
		{
			SimState->SetMatchHasStarted(true);
		}
		else
		{
			return;
		}
	}
	FRTSNumber64 realtimeseconds = GetWorld()->RealTimeSeconds;
	FRTSNumber64 deltarealtime = realtimeseconds - LastRealTimeSeconds;
	if (!bSimIsPaused)
	{
		PausedTimeSeconds += deltarealtime;
		PausedDilatedTimeSeconds += (deltarealtime * TimeScale);
		
		int32 frames = 0;
		while (ShouldAdvanceFrame() && (frames < MaxFramesPerTick))
		{
			ERTSDKShouldAdvanceInputTurnResult advanceturn = SimState->ShouldAdvanceInputTurn();
			if (advanceturn == ERTSDKShouldAdvanceInputTurnResult::Advance)
			{
				FinalizeLockstepTurn();
			}
			else if (advanceturn == ERTSDKShouldAdvanceInputTurnResult::Wait)
			{
				SetSimIsPaused(true);
				break;
			}
			AdvanceFrame();
			frames++;
		}
	}
	else
	{
		ERTSDKShouldAdvanceInputTurnResult advanceturn = SimState->ShouldAdvanceInputTurn();
		if (advanceturn == ERTSDKShouldAdvanceInputTurnResult::Advance)
		{
			FinalizeLockstepTurn();
			SetSimIsPaused(false);
		}
		else if (advanceturn == ERTSDKShouldAdvanceInputTurnResult::Skip)//unpaused from match pausing
		{
			SetSimIsPaused(false);
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

void URTSDKGameSimSubsystem::AddInputCommands(ARTSDKCommanderStateBase* inCommander, const TArray<FRTSDKPlayerCommandReplicationInfo>& inCommandInputs)
{
	TArray<TObjectPtr<URTSDKPlayerCommandBase>>& turncommands = PlayerCommandsByTurn.FindOrAdd(CurrentInputTurn + 1);
	for (int32 i = 0; i < inCommandInputs.Num(); i++)
	{
		URTSDKPlayerCommandBase* command = NewObject<URTSDKPlayerCommandBase>(this, inCommandInputs[i].Class.Get());
		command->SetAll(inCommander, this, inCommandInputs[i]);
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

struct FInputTurnFinalizeInfo
{
	ARTSDKCommanderStateBase* commander;
	int32 index; 
	TArray<FRTSDKPlayerCommandReplicationInfo> commands;

	FInputTurnFinalizeInfo(ARTSDKCommanderStateBase* cmdr, int32 i, TArray<FRTSDKPlayerCommandReplicationInfo> c)
		: commander(cmdr), index(i), commands(c)
	{}

	inline bool operator<(const FInputTurnFinalizeInfo& Other) const
	{
		return index < Other.index;
	}
};

void URTSDKGameSimSubsystem::FinalizeLockstepTurn()
{
	//flushes all the command buffers
	SimState->OnPreAdvanceInputTurn();
	//get the previous turn's commands, queue them for next turn
	TArray<FInputTurnFinalizeInfo> infos;
	for (int32 i = 0; i < SimState->GetCommanderCount(); i++)
	{
		TArray<FRTSDKPlayerCommandReplicationInfo> commands = SimState->GetCommandsForCommanderByTurn(i, CurrentInputTurn - 1);
		infos.Add(FInputTurnFinalizeInfo(SimState->GetCommander(i), i, commands));
	}
	infos.Sort();
	for (int32 i = 0; i < infos.Num(); i++)
	{
		AddInputCommands(infos[i].commander, infos[i].commands);
	}
	CurrentInputTurn++;
}
