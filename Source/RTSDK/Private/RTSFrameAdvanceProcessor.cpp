// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSFrameAdvanceProcessor.h"
//#include "MassCommonFragments.h"
#include "RTSConstants.h"
#include "RTSGameSimSubsystem.h"

URTSFrameAdvanceProcessor::URTSFrameAdvanceProcessor()
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	bRequiresGameThreadExecution = true;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::RTSPreSim;
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSMovement);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSMoveCommit);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSVisInterpolation);
}

void URTSFrameAdvanceProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
}

void URTSFrameAdvanceProcessor::ConfigureQueries()
{

}

void URTSFrameAdvanceProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	/*UWorld* world = GetWorld();
	if (world == nullptr)
	{
		return;
	}
	URTSGameSimSubsystem* sim = world->GetSubsystem<URTSGameSimSubsystem>();
	if (sim == nullptr)
	{
		return;
	}
	int32 frames = 0;
	while (sim->ShouldAdvanceFrame() && (frames < 20))
	{
		sim->AdvanceFrame();
		frames++;
	}*/
}
