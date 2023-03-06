// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKGameSimSubsystem.h"
#include "RTSDKMassModuleSettings.h"
#include "MassExecutor.h"
#include "MassEntitySubsystem.h"
#include "MassSignalSubsystem.h"

void URTSDKGameSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	bInScriptCallingMode = false;
	CommandInputsByFrame.Empty();
	ResetFrameCount();
	ResetUnits();
	SetFrameDelay(1);
	SetTimestep(32.0);
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
	const UWorld* World = GetWorld();
	if (World != nullptr && Settings != nullptr && !Settings->DumpDependencyGraphFileName.IsEmpty())
	{
		DependencyGraphFileName = FString::Printf(TEXT("%s_%s"), *Settings->DumpDependencyGraphFileName, *ToString(World->GetNetMode()));
	}
#endif // WITH_EDITOR
	FString FileName = !DependencyGraphFileName.IsEmpty() ? FString::Printf(TEXT("%s_%s"), *DependencyGraphFileName, *Settings->SimProcessingPhaseConfig.PhaseName.ToString()) : FString();
	UMassEntitySubsystem* EntitySubsystem = World->GetSubsystem<UMassEntitySubsystem>();
	EntityManager = EntitySubsystem->GetMutableEntityManager().AsShared();
	SimProcessor = NewObject<UMassCompositeProcessor>(this, Settings->SimProcessingPhaseConfig.PhaseGroupClass,
		*FString::Printf(TEXT("ProcessingPhase_%s"), *Settings->SimProcessingPhaseConfig.PhaseName.ToString()));
	SimProcessor->CopyAndSort(Settings->SimProcessingPhaseConfig, FileName);
	SimProcessor->SetProcessingPhase(EMassProcessingPhase::PrePhysics);
	SimProcessor->SetGroupName(FName(FString::Printf(TEXT("%s Group"), *Settings->SimProcessingPhaseConfig.PhaseName.ToString())));
	SimProcessor->Initialize(*this);
	bSimIsRunning = true;
}

void URTSDKGameSimSubsystem::Deinitialize()
{
	CommandInputsByFrame.Empty();
	UnitsByID.Empty();
	bSimIsRunning = false;
	bInScriptCallingMode = false;
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
	int32 frames = 0;
	while (ShouldAdvanceFrame() && (frames < 20))
	{
		AdvanceFrame();
		frames++;
	}
}

void URTSDKGameSimSubsystem::AdvanceFrame()
{
	FrameCount++;
	UMassProcessor& proc = *SimProcessor;
	FMassProcessingContext Context(EntityManager, TimeStep);
	Context.bFlushCommandBuffer = true;
	UE::Mass::Executor::Run(proc, Context);
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
	ScriptCommandBuffer.Reset();
	ScriptCommandBuffer = MakeShareable(new FMassCommandBuffer());
	bInScriptCallingMode = true;
	return *ScriptCommandBuffer.Get();
}

void URTSDKGameSimSubsystem::EndScriptCallingMode()
{
	bInScriptCallingMode = false;
	//TODO: change this to an array of buffers, advance frame flushes them after appending them all together in order, after the processor had finished and flushed the buffer
	//this is queueing up too many command buffers and it makes mass freak out about its queue being long as fuck
	EntityManager->FlushCommands(ScriptCommandBuffer);
	ScriptCommandBuffer.Reset();
}

void URTSDKGameSimSubsystem::SetGlobalGravityDirection(FVector inDir)
{
	UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(GetWorld());
	inDir.Normalize();
	SetGravityDirection(inDir);
	SignalSubsystem->SignalEntities(UE::Mass::Signals::RTSDKUnitGravityChanged, GetAllUnitEntityHandles());
}
