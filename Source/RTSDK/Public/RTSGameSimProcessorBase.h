// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "RTSGameSimProcessorInterface.h"
#include "RTSGameSimProcessorBase.generated.h"

/**
 * Base class for game sim processors. These are coordinated by RTSGameSimSubsystem.
 * Knows the last frame it processed, and provides a way to check if derived classes 
 * should continue their Execute. (See: TryAdvanceFrame, LastProcessedFrame)
 * Starts with LastProcessedFrame of -1 if Super::Initialize is called during Initialize
 */
UCLASS(abstract)
class RTSDK_API URTSGameSimProcessorBase : public UMassProcessor, public IRTSGameSimProcessorInterface
{
	GENERATED_BODY()
public:
	URTSGameSimProcessorBase() {}

	virtual void Initialize(UObject& Owner) override
	{
		LastProcessedFrame = -1;
	}
protected:
	/**
	* Checks if there is a valid world, a game sim subsystem, the sim is running,
	* and LastProcessedFrame is not equal to the sims current framecount.
	* If so, it will set LastProcessedFrame to the sims current framecount and return true.
	* If not, it will return false. 
	* 
	* Note: At the start of derived classes Execute function you must call this and check the result.
	* If false return out of Execute. Otherwise continue on.
	*/
	bool TryAdvanceFrame();
	
	//Last frame processed by this processor.
	int32 LastProcessedFrame;
};
