// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "RTSConstants.h"
#include "RTSDKFragments.generated.h"

class URTSVisRootComponent;
class UPrimitiveComponent;

/**
*
*/
USTRUCT(BlueprintType)
struct FRTSUnitIDFragment : public FMassFragment
{
	GENERATED_BODY()

	int64 UnitID;
};

/**
* 
*/
USTRUCT(BlueprintType)
struct FRTSMovementFragment : public FMassFragment
{
	GENERATED_BODY()

	//UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRTSVector64 Velocity;
	//FFixedVector64 Velocity;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRTSTransform64 PreviousTransform;
	//FFixedTransform64 PreviousTransform;

	//UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FRTSTransform64 CurrentTransform;
	//FFixedTransform64 CurrentTransform;
};

USTRUCT(BlueprintType)
struct FRTSMovementInputFragment : public FMassFragment
{
	GENERATED_BODY()

	FRTSVector64 Input;
};

USTRUCT(BlueprintType)
struct FRTSMovementCoreParamsFragment : public FMassFragment
{
	GENERATED_BODY()

	FRTSNumber64 MaxSpeed;
	FRTSNumber64 Acceleration;
	FRTSNumber64 BrakingFriction;
	FRTSNumber64 AirControl;
};

USTRUCT(BlueprintType)
struct FRTSCollisionBoundsFragment : public FMassFragment
{
	GENERATED_BODY()

	FRTSVector64 BoundsSize;
	FRTSVector64 BoundsMin;
	FRTSVector64 BoundsMax;
	FRTSVector64 FeetLocation;
};

USTRUCT(BlueprintType)
struct FRTSMovementBasisFragment : public FMassFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<UPrimitiveComponent> Basis;

	FHitResult Impact;
};

USTRUCT(BlueprintType)
struct FRTSVisRootFragment : public FMassFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<URTSVisRootComponent> VisRoot;
};

USTRUCT(BlueprintType)
struct FRTSSimRootFragment : public FMassFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<USceneComponent> SimRoot;
};
