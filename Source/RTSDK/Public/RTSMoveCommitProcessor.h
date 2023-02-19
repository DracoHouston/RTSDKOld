// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassSignalProcessorBase.h"
#include "RTSGameSimProcessorBase.h"
#include "RTSGameSimProcessorInterface.h"
#include "RTSMoveCommitProcessor.generated.h"

struct FMoveCommitInfo
{
	uint32 key;
	FRTSMovementFragment& movefragment;
	USceneComponent* simroot;
	FRTSCollisionBoundsFragment bounds;
	FRTSMovementBasisFragment& basisfragment;
	FMassEntityHandle entity;
	bool currentlybased;

	FMoveCommitInfo(USceneComponent* inRoot, FRTSMovementFragment& inMoveFragment, FRTSMovementBasisFragment& inBasis, FRTSCollisionBoundsFragment inBounds, int64 UnitID, FMassEntityHandle inEntity, bool bIsCurrentlyBased)
		: movefragment(inMoveFragment), simroot(inRoot), bounds(inBounds), basisfragment(inBasis), entity(inEntity), currentlybased(bIsCurrentlyBased)
	{
		key = GetTypeHash(UnitID);
	}

	inline bool operator<(const FMoveCommitInfo& Other) const
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

	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery BasedMoversQuery;
	FMassEntityQuery AirMoversQuery;
	TArray<FMoveCommitInfo> MoveCommitsThisFrame;
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