// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RTSDKGameSimProcessorBase.h"
#include "RTSDKMovementPostVelocityProcessors.generated.h"

/**
* Signals to collision and commit processors that something moved.
*/
UCLASS()
class URTSDKVelocityCheckProcessor : public URTSDKGameSimProcessorBase
{
	GENERATED_BODY()
public:
	URTSDKVelocityCheckProcessor();

protected:
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	FMassEntityQuery MovementQuery;
};
