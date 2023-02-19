// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSGameSimSubsystem.h"
#include "RTSDKMassModuleSettings.h"
#include "MassExecutor.h"
#include "MassEntitySubsystem.h"

void URTSGameSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	CommandInputsByFrame.Empty();
	ResetFrameCount();
	ResetUnits();
	SetFrameDelay(1);
	SetTimestep(30.0);
	SetMetersToUUScale(100.0);
	SetGravityDirection(FVector::DownVector);
	SetGravityAcceleration(9.8);
	SetTerminalVelocity(40.0);
	Super::Initialize(Collection);
}

void URTSGameSimSubsystem::PostInitialize()
{
	Super::PostInitialize();
}

void URTSGameSimSubsystem::OnWorldBeginPlay(UWorld& InWorld)
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

void URTSGameSimSubsystem::Deinitialize()
{
	CommandInputsByFrame.Empty();
	UnitsByID.Empty();
	bSimIsRunning = false;
}

TStatId URTSGameSimSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URTSGameSimSubsystem, STATGROUP_Tickables);
}

void URTSGameSimSubsystem::Tick(float DeltaTime)
{
	int32 frames = 0;
	while (ShouldAdvanceFrame() && (frames < 20))
	{
		AdvanceFrame();
		frames++;
	}
}

void URTSGameSimSubsystem::AdvanceFrame()
{
	FrameCount++;
	UMassProcessor& proc = *SimProcessor;
	FMassProcessingContext Context(EntityManager, TimeStep);
	Context.bFlushCommandBuffer = true;
	UE::Mass::Executor::Run(proc, Context);
}
