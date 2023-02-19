// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSWalkingMovementProcessor.h"
#include "RTSDKFragments.h"
//#include "RTSDKTypes.h"
//#include "MassCommonFragments.h"
//#include "MassCommonTypes.h"
//#include "FixedPointTypes.h"
//#include "RTSGameSimSubsystem.h"
#include "RTSConstants.h"

URTSWalkingMovementProcessor::URTSWalkingMovementProcessor()
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	bRequiresGameThreadExecution = false;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::RTSMovement;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::RTSPreSim);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSMovePreCommit);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSMoveCommit);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSVisInterpolation);
	bAutoRegisterWithProcessingPhases = false;
	bCanShowUpInSettings = false;
}

void URTSWalkingMovementProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
}

void URTSWalkingMovementProcessor::ConfigureQueries()
{
	MovementQuery.AddRequirement<FRTSMovementInputFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	MovementQuery.AddRequirement<FRTSMovementCoreParamsFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	MovementQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	MovementQuery.AddRequirement<FRTSMovementBasisFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);
	MovementQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);
	MovementQuery.RegisterWithProcessor(*this);
}

void URTSWalkingMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	/*if (!TryAdvanceFrame())
	{
		return;
	}*/
	MovementQuery.ForEachEntityChunk(EntityManager,	Context, [this](FMassExecutionContext& Context)
	{
		TConstArrayView<FRTSMovementInputFragment> inputs = Context.GetFragmentView<FRTSMovementInputFragment>();
		TConstArrayView<FRTSMovementCoreParamsFragment> movecoreparams = Context.GetFragmentView<FRTSMovementCoreParamsFragment>();
		TConstArrayView<FMassEntityHandle> entities = Context.GetEntities();
		TArrayView<FRTSMovementBasisFragment> bases = Context.GetMutableFragmentView<FRTSMovementBasisFragment>();
		TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
		int32 entcount = Context.GetNumEntities();
		for (int32 i = 0; i < entcount; i++)
		{
			velocities[i].Velocity += FRTSVector64::VectorPlaneProject(inputs[i].Input * movecoreparams[i].Acceleration, bases[i].Impact.ImpactNormal);
			velocities[i].Velocity = velocities[i].Velocity.SizeSquared() > FRTSMath::Square(movecoreparams[i].MaxSpeed) ? velocities[i].Velocity.GetSafeNormal() * movecoreparams[i].MaxSpeed : velocities[i].Velocity;
		}
	});
}
