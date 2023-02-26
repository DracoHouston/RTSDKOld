// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSMoveCommitProcessor.h"
#include "RTSDKFragments.h"
#include "RTSGameSimSubsystem.h"
#include "MassSignalSubsystem.h"
#include "RTSConstants.h"
#include "RTSBatchedSimCommand.h"
#include "RTSDKUnitComponent.h"

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
	EntityQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSUnitComponentFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSMovementCoreParamsFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSMovementComplexParamsFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::None);

	BasedMoversQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSUnitComponentFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSMovementBasisFragment>(EMassFragmentAccess::ReadWrite);
	BasedMoversQuery.AddRequirement<FRTSMovementCoreParamsFragment>(EMassFragmentAccess::None);
	BasedMoversQuery.AddRequirement<FRTSMovementComplexParamsFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite);

	BasedMoversQuery.RegisterWithProcessor(*this);

	AirMoversQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSUnitComponentFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSMovementBasisFragment>(EMassFragmentAccess::None, EMassFragmentPresence::None);
	AirMoversQuery.AddRequirement<FRTSMovementCoreParamsFragment>(EMassFragmentAccess::None);
	AirMoversQuery.AddRequirement<FRTSMovementComplexParamsFragment>(EMassFragmentAccess::ReadOnly);
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
	BasedMoversQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
	{
		TConstArrayView<FRTSSimRootFragment> roots = Context.GetFragmentView<FRTSSimRootFragment>();
		TConstArrayView<FRTSUnitComponentFragment> unitcomps = Context.GetFragmentView<FRTSUnitComponentFragment>();
		TConstArrayView<FRTSUnitIDFragment> unitids = Context.GetFragmentView<FRTSUnitIDFragment>();
		TConstArrayView<FRTSCollisionBoundsFragment> bounds = Context.GetFragmentView<FRTSCollisionBoundsFragment>();
		TConstArrayView<FRTSMovementComplexParamsFragment> complexparams = Context.GetFragmentView<FRTSMovementComplexParamsFragment>();
		TConstArrayView<FMassEntityHandle> entities = Context.GetEntities();
		TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
		TArrayView<FRTSMovementBasisFragment> bases = Context.GetMutableFragmentView<FRTSMovementBasisFragment>();
		int32 entcount = Context.GetNumEntities();
		for (int32 i = 0; i < entcount; i++)
		{
			FBasedMoveCommitInfo movecommit(roots[i].SimRoot.Get(), unitcomps[i].UnitComponent.Get(), velocities[i], bases[i], bounds[i], complexparams[i], unitids[i].UnitID, entities[i]);
			BasedMoveCommitsThisFrame.Add(movecommit);
			velocities[i].PreviousTransform = velocities[i].CurrentTransform;
		}
	});
	AirMoversQuery.ForEachEntityChunk(EntityManager, Context, [this](FMassExecutionContext& Context)
	{
		TConstArrayView<FRTSSimRootFragment> roots = Context.GetFragmentView<FRTSSimRootFragment>();
		TConstArrayView<FRTSUnitComponentFragment> unitcomps = Context.GetFragmentView<FRTSUnitComponentFragment>();
		TConstArrayView<FRTSUnitIDFragment> unitids = Context.GetFragmentView<FRTSUnitIDFragment>();
		TConstArrayView<FRTSCollisionBoundsFragment> bounds = Context.GetFragmentView<FRTSCollisionBoundsFragment>();
		TConstArrayView<FRTSMovementComplexParamsFragment> complexparams = Context.GetFragmentView<FRTSMovementComplexParamsFragment>();
		TConstArrayView<FMassEntityHandle> entities = Context.GetEntities();
		TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
		int32 entcount = Context.GetNumEntities();
		for (int32 i = 0; i < entcount; i++)
		{
			FAirMoveCommitInfo movecommit(roots[i].SimRoot.Get(), unitcomps[i].UnitComponent.Get(), velocities[i], bounds[i], complexparams[i], unitids[i].UnitID, entities[i]);
			AirMoveCommitsThisFrame.Add(movecommit);
			velocities[i].PreviousTransform = velocities[i].CurrentTransform;
		}
	});	
}

void URTSMoveCommitProcessor::ProcessBasedMovers(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* world = GetWorld();
	URTSGameSimSubsystem* sim = world->GetSubsystem<URTSGameSimSubsystem>();
	FRTSVector64 gravdir = sim->GetGravityDirection();
	for (int32 i = 0; i < BasedMoveCommitsThisFrame.Num(); i++)
	{
		FRTSTransform64 testtransform = BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform;
		testtransform.AddToTranslation(BasedMoveCommitsThisFrame[i].movefragment.Velocity);
		FHitResult Hit;
		FCollisionShape boundsbox = FCollisionShape::MakeCapsule(BasedMoveCommitsThisFrame[i].bounds.BoundsRadius, BasedMoveCommitsThisFrame[i].bounds.BoundsHalfHeight);
		FCollisionQueryParams collisionqueryparams;
		collisionqueryparams.AddIgnoredActor(BasedMoveCommitsThisFrame[i].simroot->GetOwner());
		collisionqueryparams.bIgnoreTouches = true;//blocking only
		if (world->SweepSingleByChannel
		(
			Hit,
			BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform.GetTranslation(),
			testtransform.GetTranslation(),
			BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform.GetRotation(),
			ECollisionChannel::ECC_GameTraceChannel2,
			boundsbox,
			collisionqueryparams
		))
		{
			BasedMoveCommitsThisFrame[i].movefragment.Velocity = FMath::IsNearlyZero(Hit.Distance) ? FRTSVector64::ZeroVector : BasedMoveCommitsThisFrame[i].movefragment.Velocity.GetSafeNormal() * ((FRTSNumber64)Hit.Distance * FRTSNumber64::Make(0.95));
			testtransform = BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform;
			testtransform.AddToTranslation(BasedMoveCommitsThisFrame[i].movefragment.Velocity);
			Context.Defer().PushCommand<FRTSBroadcastUnitOnCollidedCommand>(BasedMoveCommitsThisFrame[i].entity, BasedMoveCommitsThisFrame[i].unitcomponent, Hit);
		}
		FRTSVector64 stepupoffset = -gravdir * BasedMoveCommitsThisFrame[i].complexparams.StepUpHeight;
		FRTSVector64 stepdownoffset = gravdir * BasedMoveCommitsThisFrame[i].complexparams.StepDownHeight;
		FRTSVector64 start = testtransform.GetTranslation() + stepupoffset;
		FRTSVector64 end = testtransform.GetTranslation() + stepdownoffset;

		TArray<FHitResult> BasisHits;
		FCollisionShape basisboundsbox = FCollisionShape::MakeCapsule(BasedMoveCommitsThisFrame[i].bounds.BoundsRadius, 0.5);
		FRTSNumber64 bestfloorangle;
		FRTSNumber64 bestfloordistance;
		FHitResult bestfloorhit;
		bool foundbase = false;
		TArray<UPrimitiveComponent*> unsuitablebases;
		if (world->SweepMultiByChannel
		(
			BasisHits,
			start,
			end,
			testtransform.GetRotation(),
			ECollisionChannel::ECC_GameTraceChannel1,
			basisboundsbox,
			collisionqueryparams))
		{
			bestfloorhit = BasisHits[BasisHits.Num() - 1];
			if ((bestfloorhit.bStartPenetrating) && (bestfloorhit.PenetrationDepth > 0.0))
			{
				FRTSVector64 testpos = testtransform.GetTranslation() + ((FRTSVector64)bestfloorhit.Normal * (FRTSNumber64)bestfloorhit.PenetrationDepth);
				FRTSVector64 teststart = testpos + stepupoffset;
				FRTSVector64 testend = testpos + stepdownoffset;
				if (bestfloorhit.GetComponent()->SweepComponent(Hit, teststart, testend, testtransform.GetRotation(), basisboundsbox))
				{
					if (!Hit.bStartPenetrating)
					{
						bestfloorhit = Hit;
						bestfloordistance = (testpos - bestfloorhit.Location).SizeSquared();
						bestfloorangle = FRTSMath::Acos(-gravdir | bestfloorhit.ImpactNormal);
						if (bestfloorangle < BasedMoveCommitsThisFrame[i].complexparams.MaxWalkableAngle)
						{
							foundbase = true;
						}
						else
						{
							unsuitablebases.Add(Hit.GetComponent());
						}
					}
				}
			}
			else
			{
				bestfloordistance = (testtransform.GetTranslation() - bestfloorhit.Location).SizeSquared();
				bestfloorangle = FRTSMath::Acos(-gravdir | bestfloorhit.ImpactNormal);
				if (bestfloorangle < BasedMoveCommitsThisFrame[i].complexparams.MaxWalkableAngle)
				{
					foundbase = true;
				}
				else
				{
					unsuitablebases.Add(bestfloorhit.GetComponent());
				}
			}
			for (int32 hitsidx = BasisHits.Num() - 2; hitsidx >= 0; hitsidx--)
			{
				if (BasisHits[hitsidx].GetComponent()->SweepComponent(Hit, start, end, testtransform.GetRotation(), basisboundsbox))
				{
					if (Hit.bStartPenetrating)
					{
						if (Hit.PenetrationDepth > 0.0)
						{
							FRTSVector64 testpos = testtransform.GetTranslation() + ((FRTSVector64)Hit.Normal * (FRTSNumber64)Hit.PenetrationDepth);
							FRTSVector64 teststart = testpos + stepupoffset;
							FRTSVector64 testend = testpos + stepdownoffset;
							if (Hit.GetComponent()->SweepComponent(Hit, teststart, testend, testtransform.GetRotation(), basisboundsbox))
							{
								if (!Hit.bStartPenetrating)
								{

									FRTSNumber64 testfloordistance = (testpos - (FRTSVector64)Hit.Location).SizeSquared();
									FRTSNumber64 testfloorangle = FRTSMath::Acos(-gravdir | (FRTSVector64)Hit.ImpactNormal);
									if (testfloorangle < BasedMoveCommitsThisFrame[i].complexparams.MaxWalkableAngle)
									{
										if (testfloordistance < bestfloordistance)
										{
											bestfloorhit = Hit;
											bestfloordistance = testfloordistance;
											bestfloorangle = testfloorangle;
											foundbase = true;
										}
									}
									else
									{
										unsuitablebases.Add(Hit.GetComponent());
									}
								}
							}
						}
						else
						{
							unsuitablebases.Add(Hit.GetComponent());
						}
					}
					else
					{
						FRTSNumber64 testfloordistance = (testtransform.GetTranslation() - (FRTSVector64)Hit.Location).SizeSquared();
						FRTSNumber64 testfloorangle = FRTSMath::Acos(-gravdir | (FRTSVector64)Hit.ImpactNormal);
						if (testfloorangle < BasedMoveCommitsThisFrame[i].complexparams.MaxWalkableAngle)
						{
							if (testfloordistance < bestfloordistance)
							{
								bestfloorhit = Hit;
								bestfloordistance = testfloordistance;
								bestfloorangle = testfloorangle;
								foundbase = true;
							}
						}
						else
						{
							unsuitablebases.Add(Hit.GetComponent());
						}
					}
				}
			}
		}

		bool foundblockingunsuitablebase = false;
		FRTSNumber64 closestdistance = UE_DOUBLE_BIG_NUMBER;
		FHitResult closestblockingunsuitablebasehit;
		for (int32 blockersidx = 0; blockersidx < unsuitablebases.Num(); blockersidx++)
		{
			if (unsuitablebases[blockersidx]->SweepComponent(
				Hit, 
				BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform.GetTranslation(), 
				testtransform.GetTranslation(), 
				testtransform.GetRotation(), 
				basisboundsbox))
			{
				if ((FRTSNumber64)Hit.Distance < closestdistance)
				{
					closestdistance = Hit.Distance;
					closestblockingunsuitablebasehit = Hit;
					foundblockingunsuitablebase = true;
				}
			}
		}

		if (foundbase)
		{
			if (bestfloorhit.Component != BasedMoveCommitsThisFrame[i].basisfragment.Basis)
			{
				testtransform.SetTranslation(bestfloorhit.Location + -BasedMoveCommitsThisFrame[i].bounds.FeetLocation);
			}

			BasedMoveCommitsThisFrame[i].basisfragment.Basis = bestfloorhit.Component;
			BasedMoveCommitsThisFrame[i].basisfragment.Impact = bestfloorhit;
		}
		else
		{
			Context.Defer().RemoveFragment<FRTSMovementBasisFragment>(BasedMoveCommitsThisFrame[i].entity);
		}
		
		if (foundblockingunsuitablebase)
		{
			BasedMoveCommitsThisFrame[i].movefragment.Velocity = FMath::IsNearlyZero(closestdistance) ? FRTSVector64::ZeroVector : BasedMoveCommitsThisFrame[i].movefragment.Velocity.GetSafeNormal() * (closestdistance * FRTSNumber64::Make(0.95));
			testtransform = BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform;
			testtransform.AddToTranslation(BasedMoveCommitsThisFrame[i].movefragment.Velocity);
			Context.Defer().PushCommand<FRTSBroadcastUnitOnCollidedCommand>(BasedMoveCommitsThisFrame[i].entity, BasedMoveCommitsThisFrame[i].unitcomponent, closestblockingunsuitablebasehit);
		}

		BasedMoveCommitsThisFrame[i].simroot->SetWorldTransform(testtransform);
		BasedMoveCommitsThisFrame[i].simroot->ComponentVelocity = BasedMoveCommitsThisFrame[i].movefragment.Velocity;
		BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform = testtransform;
	}
}

void URTSMoveCommitProcessor::ProcessAirMovers(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* world = GetWorld();
	URTSGameSimSubsystem* sim = world->GetSubsystem<URTSGameSimSubsystem>();
	FRTSVector64 gravdir = sim->GetGravityDirection();
	for (int32 i = 0; i < AirMoveCommitsThisFrame.Num(); i++)
	{
		FRTSTransform64 testtransform = AirMoveCommitsThisFrame[i].movefragment.CurrentTransform;
		testtransform.AddToTranslation(AirMoveCommitsThisFrame[i].movefragment.Velocity);
		FHitResult Hit;
		FCollisionShape boundsbox = FCollisionShape::MakeCapsule(AirMoveCommitsThisFrame[i].bounds.BoundsRadius, AirMoveCommitsThisFrame[i].bounds.BoundsHalfHeight);
		FCollisionQueryParams collisionqueryparams;
		collisionqueryparams.AddIgnoredActor(AirMoveCommitsThisFrame[i].simroot->GetOwner());
		collisionqueryparams.bIgnoreTouches = true;//blocking only
		if (world->SweepSingleByChannel
		(
			Hit,
			AirMoveCommitsThisFrame[i].movefragment.CurrentTransform.GetTranslation(),
			testtransform.GetTranslation(),
			AirMoveCommitsThisFrame[i].movefragment.CurrentTransform.GetRotation(),
			ECollisionChannel::ECC_GameTraceChannel2,
			boundsbox,
			collisionqueryparams
		))
		{
			AirMoveCommitsThisFrame[i].movefragment.Velocity = FMath::IsNearlyZero(Hit.Distance) ? FRTSVector64::ZeroVector : AirMoveCommitsThisFrame[i].movefragment.Velocity.GetSafeNormal() * ((FRTSNumber64)Hit.Distance * FRTSNumber64::Make(0.95));
			testtransform = AirMoveCommitsThisFrame[i].movefragment.CurrentTransform;
			testtransform.AddToTranslation(AirMoveCommitsThisFrame[i].movefragment.Velocity);
			Context.Defer().PushCommand<FRTSBroadcastUnitOnCollidedCommand>(AirMoveCommitsThisFrame[i].entity, AirMoveCommitsThisFrame[i].unitcomponent, Hit);
		}

		FRTSVector64 stepupoffset = -gravdir * AirMoveCommitsThisFrame[i].complexparams.StepUpHeight;
		FRTSVector64 stepdownoffset = gravdir * AirMoveCommitsThisFrame[i].complexparams.StepDownHeight;
		FRTSVector64 start = testtransform.GetTranslation() + stepupoffset;
		FRTSVector64 end = testtransform.GetTranslation() + stepdownoffset;

		TArray<FHitResult> BasisHits;
		FCollisionShape basisboundsbox = FCollisionShape::MakeCapsule(AirMoveCommitsThisFrame[i].bounds.BoundsRadius, 0.5);
		FRTSNumber64 bestfloorangle;
		FRTSNumber64 bestfloordistance;
		FHitResult bestfloorhit;
		bool foundbase = false;
		bool foundunsuitablebase = false;
		if (world->SweepMultiByChannel
		(
			BasisHits,
			start,
			end,
			testtransform.GetRotation(),
			ECollisionChannel::ECC_GameTraceChannel1,
			basisboundsbox,
			collisionqueryparams
		))
		{
			bestfloorhit = BasisHits[BasisHits.Num() - 1];
			if ((bestfloorhit.bStartPenetrating) && (bestfloorhit.PenetrationDepth > 0.0))
			{
				FRTSVector64 testpos = testtransform.GetTranslation() + ((FRTSVector64)bestfloorhit.Normal * (FRTSNumber64)bestfloorhit.PenetrationDepth);
				FRTSVector64 teststart = testpos + stepupoffset;
				FRTSVector64 testend = testpos + stepdownoffset;
				if (bestfloorhit.GetComponent()->SweepComponent(Hit, teststart, testend, testtransform.GetRotation(), basisboundsbox))
				{
					if (!Hit.bStartPenetrating)
					{
						bestfloorhit = Hit;
						bestfloordistance = (testpos - (FRTSVector64)bestfloorhit.Location).SizeSquared();
						bestfloorangle = FRTSMath::Acos(-gravdir | (FRTSVector64)bestfloorhit.ImpactNormal);
						if (bestfloorangle < AirMoveCommitsThisFrame[i].complexparams.MaxWalkableAngle)
						{
							foundbase = true;
						}
						else
						{
							foundunsuitablebase = true;
						}
					}
				}
			}
			else
			{
				bestfloordistance = (testtransform.GetTranslation() - (FRTSVector64)bestfloorhit.Location).SizeSquared();
				bestfloorangle = FRTSMath::Acos(-gravdir | (FRTSVector64)bestfloorhit.ImpactNormal);
				if (bestfloorangle < AirMoveCommitsThisFrame[i].complexparams.MaxWalkableAngle)
				{
					foundbase = true;
				}
				else
				{
					foundunsuitablebase = true;
				}
			}
			for (int32 hitsidx = BasisHits.Num() - 2; hitsidx >= 0; hitsidx--)
			{
				if (BasisHits[hitsidx].GetComponent()->SweepComponent(Hit, start, end, testtransform.GetRotation(), basisboundsbox))
				{
					if ((Hit.bStartPenetrating) && (Hit.PenetrationDepth > 0.0))
					{
						FRTSVector64 testpos = testtransform.GetTranslation() + ((FRTSVector64)Hit.Normal * (FRTSNumber64)Hit.PenetrationDepth);
						FRTSVector64 teststart = testpos + stepupoffset;
						FRTSVector64 testend = testpos + stepdownoffset;
						if (bestfloorhit.GetComponent()->SweepComponent(Hit, teststart, testend, testtransform.GetRotation(), basisboundsbox))
						{
							if (!Hit.bStartPenetrating)
							{
								
								FRTSNumber64 testfloordistance = (testpos - (FRTSVector64)bestfloorhit.Location).SizeSquared();
								FRTSNumber64 testfloorangle = FRTSMath::Acos(-gravdir | (FRTSVector64)bestfloorhit.ImpactNormal);
								if ((testfloorangle < AirMoveCommitsThisFrame[i].complexparams.MaxWalkableAngle) && (testfloordistance < bestfloordistance))
								{
									bestfloorhit = Hit;
									bestfloordistance = testfloordistance;
									bestfloorangle = testfloorangle;
									foundbase = true;
								}
								else
								{
									foundunsuitablebase = true;
								}
							}
						}
					}
					else
					{
						FRTSNumber64 testfloordistance = (testtransform.GetTranslation() - (FRTSVector64)bestfloorhit.Location).SizeSquared();
						FRTSNumber64 testfloorangle = FRTSMath::Acos(-gravdir | (FRTSVector64)bestfloorhit.ImpactNormal);
						if ((testfloorangle < AirMoveCommitsThisFrame[i].complexparams.MaxWalkableAngle) && (testfloordistance < bestfloordistance))
						{
							bestfloorhit = Hit;
							bestfloordistance = testfloordistance;
							bestfloorangle = testfloorangle;
							foundbase = true;
						}
						else
						{
							foundunsuitablebase = true;
						}
					}
				}
			}
		}
		if (foundbase)
		{
			testtransform.SetTranslation(bestfloorhit.Location + -AirMoveCommitsThisFrame[i].bounds.FeetLocation);
			AirMoveCommitsThisFrame[i].movefragment.Velocity = FRTSVector64::ZeroVector;
			
			FRTSMovementBasisFragment newbasis;
			newbasis.Basis = bestfloorhit.Component;
			newbasis.Impact = bestfloorhit;
			Context.Defer().PushCommand<FMassCommandAddFragmentInstances>(AirMoveCommitsThisFrame[i].entity, newbasis);
		}
		else
		{
			if (foundunsuitablebase && (world->SweepSingleByChannel
			(
				Hit,
				AirMoveCommitsThisFrame[i].movefragment.CurrentTransform.GetTranslation(),
				testtransform.GetTranslation(),
				AirMoveCommitsThisFrame[i].movefragment.CurrentTransform.GetRotation(),
				ECollisionChannel::ECC_GameTraceChannel1,
				boundsbox,
				collisionqueryparams
			)))
			{
				AirMoveCommitsThisFrame[i].movefragment.Velocity = FMath::IsNearlyZero(Hit.Distance) ? FRTSVector64::ZeroVector : AirMoveCommitsThisFrame[i].movefragment.Velocity.GetSafeNormal() * ((FRTSNumber64)Hit.Distance * FRTSNumber64::Make(0.95));
				testtransform = AirMoveCommitsThisFrame[i].movefragment.CurrentTransform;
				testtransform.AddToTranslation(AirMoveCommitsThisFrame[i].movefragment.Velocity);
				Context.Defer().PushCommand<FRTSBroadcastUnitOnCollidedCommand>(AirMoveCommitsThisFrame[i].entity, AirMoveCommitsThisFrame[i].unitcomponent, Hit);
			}
		}

		AirMoveCommitsThisFrame[i].simroot->SetWorldTransform(testtransform);
		AirMoveCommitsThisFrame[i].simroot->ComponentVelocity = AirMoveCommitsThisFrame[i].movefragment.Velocity;
		AirMoveCommitsThisFrame[i].movefragment.CurrentTransform = testtransform;
	}
}

void URTSMoveCommitProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	BasedMoveCommitsThisFrame.Empty();
	AirMoveCommitsThisFrame.Empty();
	
	Super::Execute(EntityManager, Context);

	BasedMoveCommitsThisFrame.Sort();
	AirMoveCommitsThisFrame.Sort();
	
	ProcessBasedMovers(EntityManager, Context);
	ProcessAirMovers(EntityManager, Context);
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
