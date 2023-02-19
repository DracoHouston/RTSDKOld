// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSVisRootInterpolationTranslator.h"
#include "RTSDKFragments.h"
#include "RTSDKTypes.h"
//#include "MassCommonFragments.h"
//#include "MassCommonTypes.h"
//#include "FixedPointTypes.h"
#include "RTSGameSimSubsystem.h"
#include "RTSConstants.h"
#include "RTSVisRootComponent.h"

URTSVisRootInterpolationTranslator::URTSVisRootInterpolationTranslator()
{
	//Run only for players, client and standalone
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	bRequiresGameThreadExecution = true;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::RTSVisInterpolation;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::RTSPreSim);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::RTSMovement);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::RTSMoveCommit);
}

void URTSVisRootInterpolationTranslator::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
}

void URTSVisRootInterpolationTranslator::ConfigureQueries()
{
	InterpolationQuery.AddRequirement<FRTSVisRootFragment>(EMassFragmentAccess::ReadOnly);
	InterpolationQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite);
	InterpolationQuery.RegisterWithProcessor(*this);
}

void URTSVisRootInterpolationTranslator::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* world = GetWorld();
	if (world == nullptr)
	{
		return;
	}
	URTSGameSimSubsystem* sim = world->GetSubsystem<URTSGameSimSubsystem>();
	if (sim == nullptr)
	{
		return;
	}
	if (!sim->IsSimRunning())
	{
		return;
	}
	double alpha = FMath::Fmod(world->TimeSeconds, sim->GetTimestep());
	InterpolationQuery.ForEachEntityChunk(
		EntityManager,
		Context,
		[this, alpha](FMassExecutionContext& Context)
		{
			TConstArrayView<FRTSVisRootFragment> visroots = Context.GetFragmentView<FRTSVisRootFragment>();
			TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
			int32 entcount = Context.GetNumEntities();
			for (int32 i = 0; i < entcount; i++)
			{
				//transforms[i].GetMutableTransform().Blend(velocities[i].PreviousTransform, velocities[i].CurrentTransform, alpha);
				FTransform blendedtransform = FTransform::Identity;
				blendedtransform.Blend(velocities[i].PreviousTransform, velocities[i].CurrentTransform, alpha);
				visroots[i].VisRoot.Get()->SetWorldTransform(blendedtransform);
			}
		}
	);	
}
