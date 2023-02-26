// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "RTSConstants.h"
#include "MassCommands.h"
#include "RTSDKFragments.h"


template <typename... TArgs>
struct FRTSSetUnitInputScriptCommand : public FMassBatchedEntityCommand
{
	using Super = FMassBatchedEntityCommand;

	FRTSSetUnitInputScriptCommand()
	{
		OperationType = EMassCommandOperationType::None;
	}

	void Add(FMassEntityHandle Entity, FRTSVector64 Direction)
	{
		Super::Add(Entity);
		NewDirections.Add(Direction);
	}

	TArray<FRTSVector64> NewDirections;

	virtual void Execute(FMassEntityManager& System) const override
	{
		for (int32 i = 0; i < NewDirections.Num(); i++)
		{
			System.GetFragmentDataChecked<FRTSMovementInputFragment>(TargetEntities[i]).Input = NewDirections[i];
		}
	}
};