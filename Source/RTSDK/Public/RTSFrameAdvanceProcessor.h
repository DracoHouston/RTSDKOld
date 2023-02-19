// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "RTSFrameAdvanceProcessor.generated.h"

/**
 * Signal processor that responds to "PlayTaunt" signal
 * Plays the appropriate taunt for every pending "PlayTaunt" signal
 */
UCLASS()
class RTSDK_API URTSFrameAdvanceProcessor : public UMassProcessor
{
	GENERATED_BODY()
public:
	//sets some defaults in the class default object
	URTSFrameAdvanceProcessor();
	
	//Function where we subscribe to a signal
	virtual void Initialize(UObject& Owner) override;
protected:
	//Function that configures the queries, after Initialize is run.
	virtual void ConfigureQueries() override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
};
