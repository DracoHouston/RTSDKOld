// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSMoveCommitProcessor.h"
#include "RTSDKFragments.h"
#include "RTSGameSimSubsystem.h"
#include "MassSignalSubsystem.h"
#include "RTSConstants.h"

URTSMoveCommitProcessor::URTSMoveCommitProcessor()
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	bRequiresGameThreadExecution = true;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::RTSMoveCommit;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::RTSPreSim);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::RTSMovement);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSVisInterpolation);
	bAutoRegisterWithProcessingPhases = false;
	bCanShowUpInSettings = false;
}

void URTSMoveCommitProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
	UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(Owner.GetWorld());
	SubscribeToSignal(*SignalSubsystem, UE::Mass::Signals::RTSUnitMoved);
}

void URTSMoveCommitProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite);

	BasedMoversQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSMovementBasisFragment>(EMassFragmentAccess::ReadWrite);
	BasedMoversQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite);

	BasedMoversQuery.RegisterWithProcessor(*this);

	AirMoversQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSMovementBasisFragment>(EMassFragmentAccess::None, EMassFragmentPresence::None);
	AirMoversQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite);
	
	AirMoversQuery.RegisterWithProcessor(*this);
}



void URTSMoveCommitProcessor::SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context, FMassSignalNameLookup& EntitySignals)
{
	
	//TODO
	//add penetration check on no collision when based moving, against just the movement base's primitive component
	//look into a grid-snap system for collision results:
	//standard 'collision grid' of some power of 2 fraction of 1, such as 32, 64, 128 etc
	//use the grid snap function in frtsmath to do it under the hood, helper should exist for this
	//helper eats a hit result returns a rts hit result using deterministic math types
	//work out the distance between 2 points with FRTSVector64s and dont trust hitresult.distance
	AirMoversQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
	{
		TConstArrayView<FRTSSimRootFragment> roots = Context.GetFragmentView<FRTSSimRootFragment>();
		TConstArrayView<FRTSUnitIDFragment> unitids = Context.GetFragmentView<FRTSUnitIDFragment>();
		TConstArrayView<FRTSCollisionBoundsFragment> bounds = Context.GetFragmentView<FRTSCollisionBoundsFragment>();
		TConstArrayView<FMassEntityHandle> entities = Context.GetEntities();
		TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
		int32 entcount = Context.GetNumEntities();
		for (int32 i = 0; i < entcount; i++)
		{
			FRTSMovementBasisFragment dummybasis;
			FMoveCommitInfo movecommit(roots[i].SimRoot.Get(), velocities[i], dummybasis, bounds[i], unitids[i].UnitID, entities[i], false);
			MoveCommitsThisFrame.Add(movecommit);
			velocities[i].PreviousTransform = velocities[i].CurrentTransform;
		}
	});
	BasedMoversQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
	{
		TConstArrayView<FRTSSimRootFragment> roots = Context.GetFragmentView<FRTSSimRootFragment>();
		TConstArrayView<FRTSUnitIDFragment> unitids = Context.GetFragmentView<FRTSUnitIDFragment>();
		TConstArrayView<FRTSCollisionBoundsFragment> bounds = Context.GetFragmentView<FRTSCollisionBoundsFragment>();
		TConstArrayView<FMassEntityHandle> entities = Context.GetEntities();
		TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
		TArrayView<FRTSMovementBasisFragment> bases = Context.GetMutableFragmentView<FRTSMovementBasisFragment>();
		int32 entcount = Context.GetNumEntities();
		for (int32 i = 0; i < entcount; i++)
		{
			FMoveCommitInfo movecommit(roots[i].SimRoot.Get(), velocities[i], bases[i], bounds[i], unitids[i].UnitID, entities[i], true);
			MoveCommitsThisFrame.Add(movecommit);
			velocities[i].PreviousTransform = velocities[i].CurrentTransform;
		}
	});
	
}

void URTSMoveCommitProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	MoveCommitsThisFrame.Empty();
	Super::Execute(EntityManager, Context);

	MoveCommitsThisFrame.Sort();
	UWorld* world = GetWorld();
	for (int32 i = 0; i < MoveCommitsThisFrame.Num(); i++)
	{
		FRTSTransform64 testtransform = MoveCommitsThisFrame[i].movefragment.CurrentTransform;
		testtransform.AddToTranslation(MoveCommitsThisFrame[i].movefragment.Velocity);
		FHitResult Hit;
		FCollisionShape boundsbox = FCollisionShape::MakeBox(MoveCommitsThisFrame[i].bounds.BoundsMax * FRTSNumber64::Make(0.95));
		FCollisionQueryParams collisionqueryparams;
		collisionqueryparams.AddIgnoredActor(MoveCommitsThisFrame[i].simroot->GetOwner());
		bool requiresbase = true;
		if (MoveCommitsThisFrame[i].basisfragment.Basis.IsValid() && MoveCommitsThisFrame[i].currentlybased)
		{
			collisionqueryparams.AddIgnoredComponent(MoveCommitsThisFrame[i].basisfragment.Basis.Get());
		}
		collisionqueryparams.bIgnoreTouches = true;//blocking only
		if (world->SweepSingleByChannel
		(
			Hit,
			MoveCommitsThisFrame[i].movefragment.CurrentTransform.GetTranslation(),
			testtransform.GetTranslation(),
			MoveCommitsThisFrame[i].movefragment.CurrentTransform.GetRotation(),
			ECollisionChannel::ECC_Pawn,
			boundsbox,
			collisionqueryparams
		))
		{
			MoveCommitsThisFrame[i].movefragment.Velocity = FMath::IsNearlyZero(Hit.Distance) ? FRTSVector64::ZeroVector : MoveCommitsThisFrame[i].movefragment.Velocity.GetSafeNormal() * Hit.Distance;
			testtransform = MoveCommitsThisFrame[i].movefragment.CurrentTransform;
			testtransform.AddToTranslation(MoveCommitsThisFrame[i].movefragment.Velocity);
			if (Hit.Component.Get()->GetCollisionObjectType() == ECollisionChannel::ECC_GameTraceChannel1)
			{
				MoveCommitsThisFrame[i].basisfragment.Basis = Hit.Component;
				MoveCommitsThisFrame[i].basisfragment.Impact = Hit;
				MoveCommitsThisFrame[i].movefragment.Velocity = FRTSVector64::ZeroVector;
				requiresbase = false;
			}
		}
		//FPhysicsInterface::Overlap_Geom
		MoveCommitsThisFrame[i].simroot->SetWorldTransform(testtransform);
		MoveCommitsThisFrame[i].simroot->ComponentVelocity = MoveCommitsThisFrame[i].movefragment.Velocity;
		MoveCommitsThisFrame[i].movefragment.CurrentTransform = testtransform;
		if (!requiresbase)
		{
			continue;
		}
		FHitResult FloorHit;
		FRTSVector64 end = testtransform.GetTranslation() + MoveCommitsThisFrame[i].bounds.FeetLocation;
		collisionqueryparams.ClearIgnoredComponents();
		if (world->SweepSingleByChannel
		(
			FloorHit,
			testtransform.GetTranslation(),
			end,
			testtransform.GetRotation(),
			ECollisionChannel::ECC_GameTraceChannel1,
			boundsbox,
			collisionqueryparams
		))
		{
			MoveCommitsThisFrame[i].basisfragment.Basis = FloorHit.Component;
			MoveCommitsThisFrame[i].basisfragment.Impact = FloorHit;
			if (!MoveCommitsThisFrame[i].currentlybased)
			{
				Context.Defer().PushCommand<FMassCommandAddFragmentInstances>(MoveCommitsThisFrame[i].entity, MoveCommitsThisFrame[i].basisfragment);
				MoveCommitsThisFrame[i].movefragment.Velocity.Z = FRTSNumber64::Make(0.0);
			}
		}
		else
		{
			if (MoveCommitsThisFrame[i].currentlybased)
			{
				Context.Defer().RemoveFragment<FRTSMovementBasisFragment>(MoveCommitsThisFrame[i].entity);
			}
		}
	}
}

URTSVelocityCheckProcessor::URTSVelocityCheckProcessor()
{
	ExecutionFlags = (int32)EProcessorExecutionFlags::All;
	bRequiresGameThreadExecution = false;
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::RTSMovePreCommit;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::RTSPreSim);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::RTSMovement);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSMoveCommit);
	ExecutionOrder.ExecuteBefore.Add(UE::Mass::ProcessorGroupNames::RTSVisInterpolation);
	bAutoRegisterWithProcessingPhases = false;
	bCanShowUpInSettings = false;
}

void URTSVelocityCheckProcessor::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
}

void URTSVelocityCheckProcessor::ConfigureQueries()
{
	VelocityQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite);

	VelocityQuery.RegisterWithProcessor(*this);

	ProcessorRequirements.AddSubsystemRequirement<UMassSignalSubsystem>(EMassFragmentAccess::ReadWrite);
}

void URTSVelocityCheckProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	/*if (!TryAdvanceFrame())
	{
		return;
	}*/
	UMassSignalSubsystem& SignalSubsystem = Context.GetMutableSubsystemChecked<UMassSignalSubsystem>(EntityManager.GetWorld());
	TArray<FMassEntityHandle> movedentities;
	VelocityQuery.ForEachEntityChunk(EntityManager, Context, [this, &movedentities](FMassExecutionContext& Context)
	{
		TConstArrayView<FMassEntityHandle> entities = Context.GetEntities();
		TConstArrayView<FRTSMovementFragment> velocities = Context.GetFragmentView<FRTSMovementFragment>();
		int32 entcount = Context.GetNumEntities();
		for (int32 i = 0; i < entcount; i++)
		{
			if (!velocities[i].Velocity.IsNearlyZero())
			{
				movedentities.Add(entities[i]);
			}
		}
	});
	if (movedentities.Num() > 0)
	{
		SignalSubsystem.SignalEntities(UE::Mass::Signals::RTSUnitMoved, movedentities);
	}
}
