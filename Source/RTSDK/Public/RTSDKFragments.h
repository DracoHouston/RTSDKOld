// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "RTSConstants.h"
#include "RTSDKFragments.generated.h"

class URTSVisRootComponent;
class URTSDKUnitComponent;
class UPrimitiveComponent;

/**
*
*/
USTRUCT()
struct FRTSUnitIDFragment : public FMassFragment
{
	GENERATED_BODY()

	int64 UnitID;
};

/**
* 
*/
USTRUCT()
struct FRTSMovementFragment : public FMassFragment
{
	GENERATED_BODY()

	FRTSVector64 Velocity;
	FRTSTransform64 PreviousTransform;
	FRTSTransform64 CurrentTransform;
};

USTRUCT()
struct FRTSMovementInputFragment : public FMassFragment
{
	GENERATED_BODY()

	FRTSVector64 Input;
};

USTRUCT()
struct FRTSMovementCoreParamsFragment : public FMassFragment
{
	GENERATED_BODY()

	FRTSNumber64 MaxSpeed;
	FRTSNumber64 Acceleration;
	FRTSNumber64 BrakingFriction;
	FRTSNumber64 AirControl;
};

USTRUCT()
struct FRTSMovementComplexParamsFragment : public FMassFragment
{
	GENERATED_BODY()

	FRTSNumber64 StepUpHeight;
	FRTSNumber64 StepDownHeight;
	FRTSNumber64 MaxWalkableAngle;
};

USTRUCT()
struct FRTSPhysicsParamsFragment : public FMassFragment
{
	GENERATED_BODY()
	
	FRTSNumber64 Mass;
	FRTSNumber64 Volume;
	FRTSNumber64 Density;
};

USTRUCT()
struct FRTSCollisionBoundsFragment : public FMassFragment
{
	GENERATED_BODY()

	FRTSVector64 BoundsSize;
	FRTSVector64 BoundsMin;
	FRTSVector64 BoundsMax;
	FRTSNumber64 BoundsHalfHeight;
	FRTSNumber64 BoundsRadius;
	FRTSVector64 FeetLocation;
};

USTRUCT()
struct FRTSMovementBasisFragment : public FMassFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<UPrimitiveComponent> Basis;

	FHitResult Impact;
};

USTRUCT()
struct FRTSVisRootFragment : public FMassFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<URTSVisRootComponent> VisRoot;
};

USTRUCT()
struct FRTSSimRootFragment : public FMassFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<USceneComponent> SimRoot;
};

USTRUCT()
struct FRTSUnitComponentFragment : public FMassFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<URTSDKUnitComponent> UnitComponent;
};
