// Fill out your copyright notice in the Description page of Project Settings.


#include "RTSDKComplexMovementCollisionAndCommit.h"
#include "RTSDKFragments.h"
#include "RTSDKGameSimSubsystem.h"
#include "MassSignalSubsystem.h"
#include "RTSDKConstants.h"
#include "RTSDKBatchedSimCommand.h"
#include "RTSDKUnitComponent.h"

URTSDKComplexMovementCommit::URTSDKComplexMovementCommit()
{
	bRequiresGameThreadExecution = true;
	RTSDK_PROCESSOR_EXEC_ORDER_MOVEMENT_COMPLEX_LOCATION_COMMIT
}

void URTSDKComplexMovementCommit::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);
	UMassSignalSubsystem* SignalSubsystem = UWorld::GetSubsystem<UMassSignalSubsystem>(Owner.GetWorld());
	SubscribeToSignal(*SignalSubsystem, UE::Mass::Signals::RTSDKUnitHasVelocity);
}

void URTSDKComplexMovementCommit::ConfigureQueries()
{
	EntityQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSUnitComponentFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSMovementCoreParamsFragment>(EMassFragmentAccess::None);
	EntityQuery.AddRequirement<FRTSMovementComplexWalkingParamsFragment>(EMassFragmentAccess::None);
	EntityQuery.AddTagRequirement<FRTSComplex3DMovementTag>(EMassFragmentPresence::All);
	EntityQuery.AddTagRequirement<FRTSWalkingMovementTag>(EMassFragmentPresence::All);
	EntityQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::None);

	BasedMoversQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSUnitComponentFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSMovementBasisFragment>(EMassFragmentAccess::ReadWrite);
	BasedMoversQuery.AddRequirement<FRTSMovementCoreParamsFragment>(EMassFragmentAccess::None);
	BasedMoversQuery.AddRequirement<FRTSMovementComplexWalkingParamsFragment>(EMassFragmentAccess::ReadOnly);
	BasedMoversQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite);
	BasedMoversQuery.AddTagRequirement<FRTSComplex3DMovementTag>(EMassFragmentPresence::All);
	BasedMoversQuery.AddTagRequirement<FRTSWalkingMovementTag>(EMassFragmentPresence::All);

	BasedMoversQuery.RegisterWithProcessor(*this);

	AirMoversQuery.AddRequirement<FRTSSimRootFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSCollisionBoundsFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSUnitIDFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSUnitComponentFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSMovementBasisFragment>(EMassFragmentAccess::None, EMassFragmentPresence::None);
	AirMoversQuery.AddRequirement<FRTSMovementCoreParamsFragment>(EMassFragmentAccess::None);
	AirMoversQuery.AddRequirement<FRTSMovementComplexWalkingParamsFragment>(EMassFragmentAccess::ReadOnly);
	AirMoversQuery.AddRequirement<FRTSMovementFragment>(EMassFragmentAccess::ReadWrite);
	AirMoversQuery.AddTagRequirement<FRTSComplex3DMovementTag>(EMassFragmentPresence::All);
	AirMoversQuery.AddTagRequirement<FRTSWalkingMovementTag>(EMassFragmentPresence::All);
	
	AirMoversQuery.RegisterWithProcessor(*this);
}



void URTSDKComplexMovementCommit::SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context, FMassSignalNameLookup& EntitySignals)
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
		TConstArrayView<FRTSMovementComplexWalkingParamsFragment> complexparams = Context.GetFragmentView<FRTSMovementComplexWalkingParamsFragment>();
		TConstArrayView<FMassEntityHandle> entities = Context.GetEntities();
		TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
		TArrayView<FRTSMovementBasisFragment> bases = Context.GetMutableFragmentView<FRTSMovementBasisFragment>();
		int32 entcount = Context.GetNumEntities();
		for (int32 i = 0; i < entcount; i++)
		{
			FBasedWalkingMoveCommitInfo movecommit(roots[i].SimRoot.Get(), unitcomps[i].UnitComponent.Get(), velocities[i], bases[i], bounds[i], complexparams[i], unitids[i].UnitID, entities[i]);
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
		TConstArrayView<FRTSMovementComplexWalkingParamsFragment> complexparams = Context.GetFragmentView<FRTSMovementComplexWalkingParamsFragment>();
		TConstArrayView<FMassEntityHandle> entities = Context.GetEntities();
		TArrayView<FRTSMovementFragment> velocities = Context.GetMutableFragmentView<FRTSMovementFragment>();
		int32 entcount = Context.GetNumEntities();
		for (int32 i = 0; i < entcount; i++)
		{
			FAirWalkingMoveCommitInfo movecommit(roots[i].SimRoot.Get(), unitcomps[i].UnitComponent.Get(), velocities[i], bounds[i], complexparams[i], unitids[i].UnitID, entities[i]);
			AirMoveCommitsThisFrame.Add(movecommit);
			velocities[i].PreviousTransform = velocities[i].CurrentTransform;
		}
	});	
}

//Sweeps capsule conforming to bounds along velocity.
//Any collider with the BlocksUnits channel set to block will restrict the unit if hit.
//Their velocity will be recalculated and a collision event will be queued on the unit
FORCEINLINE_DEBUGGABLE void SweepAgainstBlocksUnits(
	UWorld* inWorld,
	URTSDKUnitComponent* inUnit,
	const FCollisionShape& inCollisionShape,
	const FMassEntityHandle& inEntity,
	const FRTSVector64& inLocation, 
	const FRTSQuat64& inRotation, 
	FRTSVector64& outNewLocation,
	FRTSVelocityFragment& outVelocityFragment,
	FMassExecutionContext& Context)
{
	FHitResult Hit;
	outNewLocation = inLocation + outVelocityFragment.Velocity;
	FCollisionQueryParams collisionqueryparams;
	collisionqueryparams.AddIgnoredActor(inUnit->GetOwner());
	collisionqueryparams.bIgnoreTouches = true;//blocking only

	if (inWorld->SweepSingleByChannel
	(
		Hit,
		inLocation,
		outNewLocation,
		inRotation,
		ECollisionChannel::ECC_GameTraceChannel2,
		inCollisionShape,
		collisionqueryparams
	))
	{
		outVelocityFragment.Velocity = FMath::IsNearlyZero(Hit.Distance) ? FRTSVector64::ZeroVector : outVelocityFragment.Velocity.GetSafeNormal() * ((FRTSNumber64)Hit.Distance * FRTSNumber64::Make(0.95));
		outNewLocation = inLocation + outVelocityFragment.Velocity;
		//TODO: this should fire after both this and update floor for this hit if there are no closer hits from unwalkable WalkableTerrain channel colliders
		//if it does it now, then the sweep for the unwalkable floors also collide they were closer and the real collision
		//this event only fires for the thing that hit something, not the things being hit and there should only be 1 thing a unit hits per frame!
		Context.Defer().PushCommand<FRTSDKBroadcastUnitOnCollidedCommand>(inEntity, inUnit, Hit);
	}
}

//Checks the floor angle against the gravity direction.
//Returns true if the walkable angle is less than or equal to the angle.
//Walkable angle is assumed to be the cos of the walkable angle.
FORCEINLINE_DEBUGGABLE bool FloorAngleCheck(const FRTSVector64& inGravityDirection, const FRTSVector64& inFloorNormal, const FRTSNumber64& inWalkableAngle, FRTSNumber64& outFloorAngle)
{
	outFloorAngle = -inGravityDirection | inFloorNormal;
	return outFloorAngle <= inWalkableAngle;
}

FORCEINLINE_DEBUGGABLE bool RetryFloor(
	UPrimitiveComponent* inComponent,
	const FRTSVector64& inGravityDirection,
	const FCollisionShape& inCollisionShape,
	const FRTSVector64& inPosition,
	const FRTSQuat64& inRotation, 
	const FRTSVector64& inStepUpOffset, 
	const FRTSVector64& inStepDownOffset,
	const FRTSNumber64& inWalkableAngle, 
	FRTSNumber64& outFloorAngle,
	FHitResult& outHitResult,
	bool& outFailedPenetrationTest)
{
	FRTSVector64 testpos = inPosition;
	FRTSVector64 teststart = testpos + inStepUpOffset;
	FRTSVector64 testend = testpos + inStepDownOffset;
	outFailedPenetrationTest = false;
	if (inComponent->SweepComponent(outHitResult, teststart, testend, inRotation, inCollisionShape))
	{
		if (!outHitResult.bStartPenetrating)
		{
			return FloorAngleCheck(inGravityDirection, (FRTSVector64)outHitResult.ImpactNormal, inWalkableAngle, outFloorAngle);
		}
		outFailedPenetrationTest = true;
	}
	return false;
}

FORCEINLINE_DEBUGGABLE void UpdateFloor(
	UWorld* inWorld,
	URTSDKUnitComponent* inUnit,
	const FCollisionShape& inCollisionShape,
	const FRTSVector64& inGravityDirection, 
	const FRTSMovementComplexWalkingParamsFragment& inWalkingParams,
	const FRTSQuat64& inRotation,
	FRTSVector64& outNewLocation,
	bool& outFoundBase,
	FHitResult& outBestFloorResult)
{
	FRTSVector64 stepupoffset = -inGravityDirection * inWalkingParams.StepUpHeight;
	FRTSVector64 stepdownoffset = inGravityDirection * inWalkingParams.StepDownHeight;

	FRTSVector64 start = outNewLocation + stepupoffset;
	FRTSVector64 end = outNewLocation + stepdownoffset;
	
	TArray<FHitResult> BasisHits;
	FCollisionQueryParams collisionqueryparams;
	collisionqueryparams.AddIgnoredActor(inUnit->GetOwner());
	collisionqueryparams.bIgnoreTouches = true;//blocking only

	FRTSNumber64 bestfloorangle;
	FRTSNumber64 bestfloordistance;
	TArray<UPrimitiveComponent*> unsuitablebases;
	//sweep between step up and step down locations against WalkableTerrain channel
	if (inWorld->SweepMultiByChannel
	(
		BasisHits,
		start,
		end,
		inRotation,
		ECollisionChannel::ECC_GameTraceChannel1,
		inCollisionShape,
		collisionqueryparams))
	{
		//First blocking hit is used if nothing better is found, and valid. It is last in the array of hits.
		outBestFloorResult = BasisHits[BasisHits.Num() - 1];
		//if it is penetrating and we have a depenetration solution
		if ((outBestFloorResult.bStartPenetrating) && (outBestFloorResult.PenetrationDepth > 0.0))
		{
			//retest against just the component, from suggested depenetration position
			FRTSVector64 testpos = (FRTSVector64)outBestFloorResult.Location + ((FRTSVector64)outBestFloorResult.Normal * (FRTSNumber64)outBestFloorResult.PenetrationDepth);
			FRTSNumber64 floorangle = 0.0;
			FHitResult retryhit;
			bool failedpenetrationtest;
			if (RetryFloor(outBestFloorResult.GetComponent(), 
				inGravityDirection, 
				inCollisionShape, 
				testpos, 
				inRotation, 
				stepupoffset, 
				stepdownoffset, 
				inWalkingParams.MaxWalkableAngle, 
				floorangle, 
				retryhit,
				failedpenetrationtest))
			{
				//valid, setting as best
				outBestFloorResult = retryhit;
				bestfloordistance = (testpos - outBestFloorResult.Location).SizeSquared();
				bestfloorangle = floorangle;
				outFoundBase = true;
			}
			else
			{
				//invalid, adding for collision
				unsuitablebases.Add(retryhit.GetComponent());
			}			
		}
		else //not penetrating
		{
			//Is the floor valid for walking angle?
			FRTSNumber64 floorangle;
			if (FloorAngleCheck(inGravityDirection, (FRTSVector64)outBestFloorResult.ImpactNormal, inWalkingParams.MaxWalkableAngle, floorangle))
			{
				//yes
				bestfloordistance = (outNewLocation - outBestFloorResult.Location).SizeSquared();
				bestfloorangle = floorangle;
				outFoundBase = true;
			}
			else
			{
				//no, adding for collision
				unsuitablebases.Add(outBestFloorResult.GetComponent());
			}
		}
		//For each other hit in the multi sweep we retry against the component
		for (int32 hitsidx = BasisHits.Num() - 2; hitsidx >= 0; hitsidx--)
		{
			FRTSNumber64 floorangle = 0.0;
			FHitResult otherhit;
			bool failedpenetrationtest;
			if (RetryFloor(
				BasisHits[hitsidx].GetComponent(), 
				inGravityDirection, 
				inCollisionShape, 
				outNewLocation, 
				inRotation, 
				stepupoffset, 
				stepdownoffset, 
				inWalkingParams.MaxWalkableAngle, 
				floorangle, 
				otherhit,
				failedpenetrationtest))
			{
				//valid, closer than current best?
				FRTSNumber64 dist = (outNewLocation - otherhit.Location).SizeSquared();
				if (dist < bestfloordistance)
				{
					//setting as best
					outBestFloorResult = otherhit;
					bestfloordistance = dist;
					bestfloorangle = floorangle;
					outFoundBase = true;
				}
			}
			else 
			{
				//if we failed because of penetration test and not because of walkable angle, and there is a depenetration solution, retry there
				if (failedpenetrationtest && (otherhit.PenetrationDepth > 0.0))
				{
					FRTSVector64 testpos = (FRTSVector64)otherhit.Location + ((FRTSVector64)otherhit.Normal * (FRTSNumber64)otherhit.PenetrationDepth);
					FHitResult retryhit;
					if (RetryFloor(otherhit.GetComponent(),
						inGravityDirection,
						inCollisionShape,
						testpos,
						inRotation,
						stepupoffset,
						stepdownoffset,
						inWalkingParams.MaxWalkableAngle,
						floorangle,
						retryhit,
						failedpenetrationtest))
					{
						//valid, closer?
						FRTSNumber64 dist = (outNewLocation - retryhit.Location).SizeSquared();
						if (dist < bestfloordistance)
						{
							//setting as best
							outBestFloorResult = retryhit;
							bestfloordistance = dist;
							bestfloorangle = floorangle;
							outFoundBase = true;
						}
					}
					else
					{
						//invalid, adding for collision
						unsuitablebases.Add(retryhit.GetComponent());
					}
				}
				else
				{
					//unwalkable, collide with it
					unsuitablebases.Add(otherhit.GetComponent());
				}
			}
		}

		//for each unwalkable WalkableTerrain component hit, sweep them as collision
		FHitResult Hit;
		//outNewLocation = inLocation + outVelocityFragment.Velocity;
	}
}

void URTSDKComplexMovementCommit::ProcessBasedMovers(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* world = GetWorld();
	URTSDKGameSimSubsystem* sim = world->GetSubsystem<URTSDKGameSimSubsystem>();
	FRTSVector64 gravdir = sim->GetGravityDirection();
	for (int32 i = 0; i < BasedMoveCommitsThisFrame.Num(); i++)
	{
		FCollisionShape boundsbox = FCollisionShape::MakeCapsule(BasedMoveCommitsThisFrame[i].Bounds.BoundsRadius, BasedMoveCommitsThisFrame[i].Bounds.BoundsHalfHeight);
		FRTSVector64 testlocation;
		FRTSQuat64 rotquat = BasedMoveCommitsThisFrame[i].Rotation.Rotation.Quaternion();
		SweepAgainstBlocksUnits(
			world,
			BasedMoveCommitsThisFrame[i].UnitComponent,
			boundsbox,
			BasedMoveCommitsThisFrame[i].Entity,
			BasedMoveCommitsThisFrame[i].Location.Location, 
			rotquat,
			testlocation,
			BasedMoveCommitsThisFrame[i].Velocity,
			Context);
		
		FRTSVector64 stepupoffset = -gravdir * BasedMoveCommitsThisFrame[i].WalkingParams.StepUpHeight;
		FRTSVector64 stepdownoffset = gravdir * BasedMoveCommitsThisFrame[i].WalkingParams.StepDownHeight;
		FRTSVector64 start = testlocation + stepupoffset;
		FRTSVector64 end = testlocation + stepdownoffset;

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
			BasedMoveCommitsThisFrame[i].movefragment.Velocity = FRTSMath::IsNearlyZero(closestdistance) ? FRTSVector64::ZeroVector : BasedMoveCommitsThisFrame[i].movefragment.Velocity.GetSafeNormal() * (closestdistance * FRTSNumber64::Make(0.95));
			testtransform = BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform;
			testtransform.AddToTranslation(BasedMoveCommitsThisFrame[i].movefragment.Velocity);
			Context.Defer().PushCommand<FRTSDKBroadcastUnitOnCollidedCommand>(BasedMoveCommitsThisFrame[i].entity, BasedMoveCommitsThisFrame[i].unitcomponent, closestblockingunsuitablebasehit);
		}
		FTransform othertesttransform = testtransform;
		FQuat otherrotationquat = othertesttransform.GetRotation();
		FRotator otherrot = otherrotationquat.Rotator();
		FRotator newotherrot = otherrot + (FRotator)BasedMoveCommitsThisFrame[i].movefragment.AngularVelocity;
		newotherrot.Normalize();
		FQuat newotherquat = newotherrot.Quaternion();

		FRTSQuat64 rotationquat = testtransform.GetRotation();
		FRTSQuat64 yawquat = FRTSQuat64(FRTSVector64::UpVector, FRTSMath::DegreesToRadians(BasedMoveCommitsThisFrame[i].movefragment.AngularVelocity.Yaw));
		FRTSRotator64 yawrotator = yawquat.Rotator();//yaw
		FRTSQuat64 rollquat = FRTSQuat64(FRTSVector64::BackwardVector, FRTSMath::DegreesToRadians(BasedMoveCommitsThisFrame[i].movefragment.AngularVelocity.Roll));
		FRTSRotator64 rollrotator = rollquat.Rotator();//roll, swap
		FRTSQuat64 pitchquat = FRTSQuat64(FRTSVector64::LeftVector, FRTSMath::DegreesToRadians(BasedMoveCommitsThisFrame[i].movefragment.AngularVelocity.Pitch));
		FRTSRotator64 pitchrotator = pitchquat.Rotator();//pitch, swap
		FRTSQuat64 combinedquat = BasedMoveCommitsThisFrame[i].movefragment.AngularVelocity.Quaternion();
		FRTSRotator64 combinedrotator = combinedquat.Rotator();
		FRTSRotator64 resultrotator = rotationquat.Rotator();
		/*rotationquat *= rollquat;
		rotationquat.Normalize();
		resultrotator = rotationquat.Rotator();
		rotationquat *= yawquat;
		rotationquat.Normalize();
		resultrotator = rotationquat.Rotator();
		rotationquat *= pitchquat;
		rotationquat.Normalize();
		resultrotator = rotationquat.Rotator();*/
		//rotationquat  = rotationquat.Inverse() * combinedquat;
		//rotationquat = pitchquat * rotationquat;
		//rotationquat.Normalize();
		//resultrotator = rotationquat.Rotator();
		//rotationquat = yawquat * rotationquat;
		//rotationquat.Normalize();
		//resultrotator = rotationquat.Rotator();
		//rotationquat = rollquat * rotationquat;
		//rotationquat.Normalize();
		//resultrotator = rotationquat.Rotator();

		//testtransform.SetRotation(rotationquat);

		//FRTSQuat64 rotationquat = testtransform.GetRotation();
		//FRTSRotator64 rot = rotationquat.Rotator();
		//FRTSRotator64 newrot = rot + BasedMoveCommitsThisFrame[i].movefragment.AngularVelocity;
		////newrot.Normalize();
		//FRTSQuat64 newquat = newrot.Quaternion();
		////testtransform.SetRotation(newquat);
		////testtransform.SetRotation((testtransform.GetRotation().Rotator() + BasedMoveCommitsThisFrame[i].movefragment.AngularVelocity).GetNormalized().Quaternion());
		///*testtransform.ConcatenateRotation(BasedMoveCommitsThisFrame[i].movefragment.AngularVelocity.Quaternion());
		//testtransform.NormalizeRotation();*/
		BasedMoveCommitsThisFrame[i].simroot->SetWorldLocation(testtransform.GetTranslation());
		BasedMoveCommitsThisFrame[i].simroot->AddLocalRotation(combinedquat);
		testtransform.SetRotation(BasedMoveCommitsThisFrame[i].simroot->GetRelativeTransform().GetRotation());
		BasedMoveCommitsThisFrame[i].simroot->ComponentVelocity = BasedMoveCommitsThisFrame[i].movefragment.Velocity;
		BasedMoveCommitsThisFrame[i].movefragment.CurrentTransform = testtransform;
	}
}

void URTSDKComplexMovementCommit::ProcessAirMovers(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UWorld* world = GetWorld();
	URTSDKGameSimSubsystem* sim = world->GetSubsystem<URTSDKGameSimSubsystem>();
	FRTSVector64 gravdir = sim->GetGravityDirection();
	for (int32 i = 0; i < AirMoveCommitsThisFrame.Num(); i++)
	{
		FCollisionShape boundsbox = FCollisionShape::MakeCapsule(AirMoveCommitsThisFrame[i].Bounds.BoundsRadius, AirMoveCommitsThisFrame[i].Bounds.BoundsHalfHeight);
		FRTSVector64 testlocation;
		FRTSQuat64 rotquat = BasedMoveCommitsThisFrame[i].Rotation.Rotation.Quaternion();
		SweepAgainstBlocksUnits(
			world,
			AirMoveCommitsThisFrame[i].UnitComponent,
			boundsbox,
			AirMoveCommitsThisFrame[i].Entity,
			AirMoveCommitsThisFrame[i].Location.Location,
			rotquat,
			testlocation,
			AirMoveCommitsThisFrame[i].Velocity,
			Context);

		FRTSVector64 stepupoffset = -gravdir * AirMoveCommitsThisFrame[i].WalkingParams.StepUpHeight;
		FRTSVector64 stepdownoffset = gravdir * AirMoveCommitsThisFrame[i].WalkingParams.StepDownHeight;
		FRTSVector64 start = testlocation + stepupoffset;
		FRTSVector64 end = testlocation + stepdownoffset;

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
				Context.Defer().PushCommand<FRTSDKBroadcastUnitOnCollidedCommand>(AirMoveCommitsThisFrame[i].entity, AirMoveCommitsThisFrame[i].unitcomponent, Hit);
			}
		}
		FTransform othertesttransform = testtransform;
		FQuat otherrotationquat = othertesttransform.GetRotation();
		FRotator otherrot = otherrotationquat.Rotator();
		FRotator newotherrot = otherrot + (FRotator)AirMoveCommitsThisFrame[i].movefragment.AngularVelocity;
		newotherrot.Normalize();
		FQuat newotherquat = newotherrot.Quaternion();

		/*FRTSQuat64 rotationquat = testtransform.GetRotation();
		FRTSQuat64 pitchquat = FRTSQuat64(FRTSVector64::UpVector, FRTSMath::DegreesToRadians(AirMoveCommitsThisFrame[i].movefragment.AngularVelocity.Pitch));
		FRTSQuat64 yawquat = FRTSQuat64(FRTSVector64::ForwardVector, FRTSMath::DegreesToRadians(AirMoveCommitsThisFrame[i].movefragment.AngularVelocity.Yaw));
		FRTSQuat64 rollquat = FRTSQuat64(FRTSVector64::RightVector, FRTSMath::DegreesToRadians(AirMoveCommitsThisFrame[i].movefragment.AngularVelocity.Roll));*/
		FRTSQuat64 rotationquat = testtransform.GetRotation();
		FRTSQuat64 yawquat = FRTSQuat64(FRTSVector64::UpVector, FRTSMath::DegreesToRadians(AirMoveCommitsThisFrame[i].movefragment.AngularVelocity.Yaw));
		FRTSRotator64 yawrotator = yawquat.Rotator();//yaw
		FRTSQuat64 rollquat = FRTSQuat64(FRTSVector64::BackwardVector, FRTSMath::DegreesToRadians(AirMoveCommitsThisFrame[i].movefragment.AngularVelocity.Roll));
		FRTSRotator64 rollrotator = rollquat.Rotator();//roll, swap
		FRTSQuat64 pitchquat = FRTSQuat64(FRTSVector64::LeftVector, FRTSMath::DegreesToRadians(AirMoveCommitsThisFrame[i].movefragment.AngularVelocity.Pitch));
		FRTSRotator64 pitchrotator = pitchquat.Rotator();//pitch, swap

		FRTSQuat64 combinedquat = AirMoveCommitsThisFrame[i].movefragment.AngularVelocity.Quaternion();
		rotationquat *= pitchquat;
		rotationquat.Normalize();
		rotationquat *= yawquat;
		rotationquat.Normalize();
		rotationquat *= rollquat;
		rotationquat.Normalize();
		/*rotationquat = pitchquat * rotationquat;
		rotationquat.Normalize();
		rotationquat = yawquat * rotationquat;
		rotationquat.Normalize();
		rotationquat = rollquat * rotationquat;
		rotationquat.Normalize();*/

		//testtransform.SetRotation(rotationquat);
		//FRTSRotator64 rot = rotationquat.Rotator();
		//FRTSRotator64 newrot = rot + AirMoveCommitsThisFrame[i].movefragment.AngularVelocity;
		//newrot.Normalize();
		//FRTSQuat64 newquat = newrot.Quaternion();
		//testtransform.SetRotation(rotationquat);
		//testtransform.SetRotation((testtransform.GetRotation().Rotator() + AirMoveCommitsThisFrame[i].movefragment.AngularVelocity).Quaternion());
		//testtransform.ConcatenateRotation(AirMoveCommitsThisFrame[i].movefragment.AngularVelocity.Quaternion());
		//testtransform.NormalizeRotation();
		AirMoveCommitsThisFrame[i].simroot->SetWorldLocation(testtransform.GetTranslation());
		AirMoveCommitsThisFrame[i].simroot->AddLocalRotation(combinedquat);
		testtransform = AirMoveCommitsThisFrame[i].simroot->GetComponentTransform();
		AirMoveCommitsThisFrame[i].simroot->ComponentVelocity = AirMoveCommitsThisFrame[i].movefragment.Velocity;
		AirMoveCommitsThisFrame[i].movefragment.CurrentTransform = testtransform;
	}
}

void URTSDKComplexMovementCommit::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	BasedMoveCommitsThisFrame.Empty();
	AirMoveCommitsThisFrame.Empty();
	
	Super::Execute(EntityManager, Context);

	BasedMoveCommitsThisFrame.Sort();
	AirMoveCommitsThisFrame.Sort();
	
	ProcessBasedMovers(EntityManager, Context);
	ProcessAirMovers(EntityManager, Context);
}
