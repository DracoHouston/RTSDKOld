// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassSignalProcessorBase.h"
#include "RTSDKGameSimProcessorBase.h"
#include "RTSDKGameSimProcessorInterface.h"
#include "RTSDKComplexMovementCollisionAndCommit.generated.h"

struct FBasedWalkingMoveCommitInfo
{
	uint32 Key;
	
	FRTSCurrentLocationFragment& Location;
	FRTSCurrentRotationFragment& Rotation;
	FRTSCurrentScaleFragment& Scale;
	FRTSVelocityFragment& Velocity;
	FRTSMovementBasisFragment& Basis;

	const FRTSCollisionBoundsFragment& Bounds;
	const FRTSMovementComplexWalkingParamsFragment& WalkingParams;

	FMassEntityHandle Entity;

	USceneComponent* SimRoot;
	URTSDKUnitComponent* UnitComponent;

	FBasedWalkingMoveCommitInfo(
		USceneComponent* inRoot, 
		URTSDKUnitComponent* inUnit, 
		FRTSCurrentLocationFragment& inLocFragment, 
		FRTSCurrentRotationFragment& inRotFragment, 
		FRTSCurrentScaleFragment& inScaleFragment, 
		FRTSVelocityFragment& inVelocityFragment,
		FRTSMovementBasisFragment& inBasis, 
		const FRTSCollisionBoundsFragment& inBounds, 
		const FRTSMovementComplexWalkingParamsFragment& inMovementComplexWalkingParams, 
		int64 UnitID, 
		FMassEntityHandle inEntity)
		: Location(inLocFragment), 
		Rotation(inRotFragment), 
		Scale(inScaleFragment), 
		Velocity(inVelocityFragment), 
		Basis(inBasis), 
		Bounds(inBounds), 
		WalkingParams(inMovementComplexWalkingParams), 
		Entity(inEntity), 
		SimRoot(inRoot), 
		UnitComponent(inUnit)
	{
		Key = UnitID;
	}

	inline bool operator<(const FBasedWalkingMoveCommitInfo& Other) const
	{
		return Key < Other.Key;
	}
};

struct FAirWalkingMoveCommitInfo
{
	uint32 Key;

	FRTSCurrentLocationFragment& Location;
	FRTSCurrentRotationFragment& Rotation;
	FRTSCurrentScaleFragment& Scale;
	FRTSVelocityFragment& Velocity;

	const FRTSCollisionBoundsFragment& Bounds;
	const FRTSMovementComplexWalkingParamsFragment& WalkingParams;

	FMassEntityHandle Entity;

	USceneComponent* SimRoot;
	URTSDKUnitComponent* UnitComponent;

	FAirWalkingMoveCommitInfo(
		USceneComponent* inRoot, 
		URTSDKUnitComponent* inUnit, 
		FRTSCurrentLocationFragment& inLocFragment, 
		FRTSCurrentRotationFragment& inRotFragment, 
		FRTSCurrentScaleFragment& inScaleFragment, 
		FRTSVelocityFragment& inVelocityFragment, 
		const FRTSCollisionBoundsFragment& inBounds, 
		const FRTSMovementComplexWalkingParamsFragment& inMovementComplexParams, 
		int64 UnitID, 
		FMassEntityHandle inEntity)
		: Location(inLocFragment), 
		Rotation(inRotFragment), 
		Scale(inScaleFragment), 
		Velocity(inVelocityFragment), 
		Bounds(inBounds), 
		WalkingParams(inMovementComplexParams), 
		Entity(inEntity), 
		SimRoot(inRoot), 
		UnitComponent(inUnit)
	{
		//todo: make a 64 bit hash out of this, frame count and some constant
		Key = UnitID;
	}

	inline bool operator<(const FAirWalkingMoveCommitInfo& Other) const
	{
		return Key < Other.Key;
	}
};

/**
*
*/
UCLASS()
class URTSDKComplexMovementCommit : public UMassSignalProcessorBase, public IRTSDKGameSimProcessorInterface
{
	GENERATED_BODY()
public:
	URTSDKComplexMovementCommit();
	
	virtual void Initialize(UObject& Owner) override;
protected:
	virtual void ConfigureQueries() override;
	virtual void SignalEntities(FMassEntityManager& EntityManager, FMassExecutionContext& Context, FMassSignalNameLookup& EntitySignals) override;

	void ProcessBasedMovers(FMassEntityManager& EntityManager, FMassExecutionContext& Context);

	void ProcessAirMovers(FMassEntityManager& EntityManager, FMassExecutionContext& Context);

	bool UpdateMovementBase() {}

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery BasedMoversQuery;
	FMassEntityQuery AirMoversQuery;
	TArray<FBasedWalkingMoveCommitInfo> BasedMoveCommitsThisFrame;
	TArray<FAirWalkingMoveCommitInfo> AirMoveCommitsThisFrame;
};
