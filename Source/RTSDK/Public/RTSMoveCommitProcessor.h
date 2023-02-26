// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassSignalProcessorBase.h"
#include "RTSGameSimProcessorBase.h"
#include "RTSGameSimProcessorInterface.h"
#include "RTSMoveCommitProcessor.generated.h"

struct FBasedMoveCommitInfo
{
	uint32 key;
	
	FRTSMovementFragment& movefragment;
	FRTSMovementBasisFragment& basisfragment;

	const FRTSCollisionBoundsFragment& bounds;
	const FRTSMovementComplexParamsFragment& complexparams;

	FMassEntityHandle entity;

	USceneComponent* simroot;
	URTSDKUnitComponent* unitcomponent;

	FBasedMoveCommitInfo(USceneComponent* inRoot, URTSDKUnitComponent* inUnit, FRTSMovementFragment& inMoveFragment, FRTSMovementBasisFragment& inBasis, const FRTSCollisionBoundsFragment& inBounds, const FRTSMovementComplexParamsFragment& inMovementComplexParams, int64 UnitID, FMassEntityHandle inEntity)
		: movefragment(inMoveFragment), basisfragment(inBasis), bounds(inBounds), complexparams(inMovementComplexParams), entity(inEntity), simroot(inRoot), unitcomponent(inUnit)
	{
		key = UnitID;
	}

	inline bool operator<(const FBasedMoveCommitInfo& Other) const
	{
		return key < Other.key;
	}
};

struct FAirMoveCommitInfo
{
	uint32 key;
	FRTSMovementFragment& movefragment;
	USceneComponent* simroot;
	URTSDKUnitComponent* unitcomponent;
	const FRTSCollisionBoundsFragment& bounds;
	const FRTSMovementComplexParamsFragment& complexparams;
	FMassEntityHandle entity;

	FAirMoveCommitInfo(USceneComponent* inRoot, URTSDKUnitComponent* inUnit, FRTSMovementFragment& inMoveFragment, const FRTSCollisionBoundsFragment& inBounds, const FRTSMovementComplexParamsFragment& inMovementComplexParams, int64 UnitID, FMassEntityHandle inEntity)
		: movefragment(inMoveFragment), simroot(inRoot), unitcomponent(inUnit), bounds(inBounds), complexparams(inMovementComplexParams), entity(inEntity)
	{
		//todo: make a 64 bit hash out of this, frame count and some constant
		key = UnitID;
	}

	inline bool operator<(const FAirMoveCommitInfo& Other) const
	{
		return key < Other.key;
	}
};

/**
*
*/
UCLASS()
class URTSMoveCommitProcessor : public UMassSignalProcessorBase, public IRTSGameSimProcessorInterface
{
	GENERATED_BODY()
public:
	URTSMoveCommitProcessor();
	
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
	TArray<FBasedMoveCommitInfo> BasedMoveCommitsThisFrame;
	TArray<FAirMoveCommitInfo> AirMoveCommitsThisFrame;
};

/**
*
*/
UCLASS()
class URTSVelocityCheckProcessor : public URTSGameSimProcessorBase
{
	GENERATED_BODY()
public:
	URTSVelocityCheckProcessor();

	virtual void Initialize(UObject& Owner) override;
protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery VelocityQuery;
};