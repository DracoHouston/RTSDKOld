// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSFallingMovementProcessor.h"
#include "RTSDKFragments.h"
//#include "RTSDKTypes.h"
//#include "MassCommonFragments.h"
//#include "MassCommonTypes.h"
//#include "FixedPointTypes.h"
#include "RTSGameSimSubsystem.h"
#include "RTSConstants.h"

URTSFallingMovementProcessor::URTSFallingMovementProcessor()
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

void URTSFallingMovementProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
}

void URTSFallingMovementProcessor::ConfigureQueries()
{
	MovementQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	MovementQuery.AddRequirement<FRTSMovementInputFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	MovementQuery.AddRequirement<FRTSMovementCoreParamsFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	//MovementQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::All);
	MovementQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::All);
	MovementQuery.AddRequirement<FRTSMovementBasisFragment>(EMassFragmentAccess::None, EMassFragmentPresence::None);
	MovementQuery.RegisterWithProcessor(*this);
}

void URTSFallingMovementProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	/*if (!TryAdvanceFrame())
	{
		return;
	}*/
	URTSGameSimSubsystem* sim = GetWorld()->GetSubsystem<URTSGameSimSubsystem>();
	
	const FRTSVector64 grav = sim->GetGravityVector();
	const FRTSNumber64 terminalvelocity = sim->GetTerminalVelocity();
	const FRTSNumber64 terminalvelocitysquared = FRTSMath::Square(terminalvelocity);
	MovementQuery.ForEachEntityChunk(
		EntityManager,
		Context,
		[this, grav, terminalvelocity, terminalvelocitysquared](FMassExecutionContext& Context)
		{
			TConstArrayView<FRTSMovementCoreParamsFragment> coreparams = Context.GetFragmentView<FRTSMovementCoreParamsFragment>();
			TConstArrayView<FRTSMovementInputFragment> inputs = Context.GetFragmentView<FRTSMovementInputFragment>();
			TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
			TConstArrayView <FMassEntityHandle> entities = Context.GetEntities();
			
			const int32 entcount = Context.GetNumEntities();
			for (int32 i = 0; i < entcount; i++)
			{
				FRTSVector64 acceleration = inputs[i].Input * (coreparams[i].Acceleration * coreparams[i].AirControl);
				FRTSVector64 testvelocity = velocities[i].Velocity + acceleration;
				velocities[i].Velocity = testvelocity.SizeSquared2D() > FRTSMath::Square(coreparams[i].MaxSpeed) ? velocities[i].Velocity : testvelocity;
				velocities[i].Velocity += grav;
				velocities[i].Velocity = velocities[i].Velocity.SizeSquared() > terminalvelocitysquared ? velocities[i].Velocity.GetSafeNormal() * terminalvelocity : velocities[i].Velocity;
			}
		}
	);
}
